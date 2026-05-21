#include <opencv2/opencv.hpp>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <iostream>
#include <vector>
#include <algorithm>

using namespace cv;
using namespace std;


static tesseract::TessBaseAPI* gTess = nullptr;

bool initTesseract() {
    gTess = new tesseract::TessBaseAPI();
    if (gTess->Init(nullptr, "eng", tesseract::OEM_LSTM_ONLY)) {
        cerr << "Błąd inicjalizacji Tesseract!\n";
        return false;
    }
    gTess->SetPageSegMode(tesseract::PSM_SINGLE_CHAR);
    gTess->SetVariable("tessedit_char_whitelist", "123456789");
    gTess->SetVariable("classify_bln_numeric_mode", "1");
    gTess->SetVariable("tessedit_enable_doc_dict", "0");
    gTess->SetVariable("tessedit_enable_bigram_correction", "0");
    return true;
}

void cleanupTesseract() {
    if (gTess) { gTess->End(); delete gTess; gTess = nullptr; }
}


static vector<Point2f> kolejnosc_rogow(const vector<Point2f>& pts) {
    vector<Point2f> res(4);
    auto suma     = [](const Point2f& p) { return p.x + p.y; };
    auto roznica  = [](const Point2f& p) { return p.x - p.y; };
    int tl = 0, br = 0, tr = 0, bl = 0;
    for (int i = 1; i < 4; ++i) {
        if (suma(pts[i]) < suma(pts[tl])) tl = i;
        if (suma(pts[i]) > suma(pts[br])) br = i;
    }
    vector<int> rest;
    for (int i = 0; i < 4; ++i) if (i != tl && i != br) rest.push_back(i);
    tr = (roznica(pts[rest[0]]) > roznica(pts[rest[1]])) ? rest[0] : rest[1];
    bl = (rest[0] == tr) ? rest[1] : rest[0];
    res[0] = pts[tl]; res[1] = pts[tr];
    res[2] = pts[br]; res[3] = pts[bl];
    return res;
}


static Mat popraw_kontrast(const Mat& bgr) {
    Mat lab;
    cvtColor(bgr, lab, COLOR_BGR2Lab);
    vector<Mat> ch;
    split(lab, ch);
    Ptr<CLAHE> clahe = createCLAHE(2.0, Size(8, 8));
    clahe->apply(ch[0], ch[0]);
    merge(ch, lab);
    Mat out;
    cvtColor(lab, out, COLOR_Lab2BGR);
    return out;
}


static bool siatka_sudoku(const Mat& imgGray, vector<Point2f>& quad) {
    Mat rozmazane, binarne;
    GaussianBlur(imgGray, rozmazane, Size(7, 7), 0);
    adaptiveThreshold(rozmazane, binarne, 255,
                      ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY_INV, 15, 3);

    Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3));
    morphologyEx(binarne, binarne, MORPH_CLOSE, kernel, Point(-1, -1), 2);

    vector<vector<Point>> kontury;
    findContours(binarne, kontury, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    if (kontury.empty()) return false;

    size_t maxIdx = 0; double maxArea = 0;
    for (size_t i = 0; i < kontury.size(); i++) {
        double a = contourArea(kontury[i]);
        if (a > maxArea) { maxArea = a; maxIdx = i; }
    }

    double imgArea = (double)imgGray.cols * imgGray.rows;
    if (maxArea < imgArea * 0.10) {
        cerr << "Kontur planszy za mały (maxArea=" << maxArea << ")!\n";
        return false;
    }

    vector<Point> urposzcz;
    approxPolyDP(kontury[maxIdx], urposzcz,
                 0.02 * arcLength(kontury[maxIdx], true), true);

    vector<Point2f> corners;
    if (urposzcz.size() == 4) {
        for (auto& p : urposzcz) corners.push_back(Point2f((float)p.x, (float)p.y));
    } else {
        RotatedRect rr = minAreaRect(kontury[maxIdx]);
        Point2f box[4]; rr.points(box);
        corners.assign(box, box + 4);
    }

    quad = kolejnosc_rogow(corners);

    float d01 = (float)norm(quad[0] - quad[1]);
    float d12 = (float)norm(quad[1] - quad[2]);
    if (d01 < 1.f || d12 < 1.f) return false;
    float ratio = max(d01, d12) / min(d01, d12);
    if (ratio > 1.6f) {
        cerr << "Wykryty czworokąt nie jest kwadratem (ratio=" << ratio << ")!\n";
        return false;
    }

    return true;
}


static char rozpoznanie_cyfry(const Mat& cellBGR) {
    Mat gray;
    cvtColor(cellBGR, gray, COLOR_BGR2GRAY);

    Mat blur;
    GaussianBlur(gray, blur, Size(3, 3), 0);

    Mat th;
    threshold(blur, th, 0, 255, THRESH_BINARY_INV | THRESH_OTSU);

    Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(3, 3));
    Mat th_clean;
    morphologyEx(th, th_clean, MORPH_OPEN, kernel);

    double inkRatio = (double)countNonZero(th_clean) / (th_clean.cols * th_clean.rows);
    if (inkRatio < 0.04) return '.';   

    Mat maly;
    resize(th, maly, Size(28, 28), 0, 0, INTER_AREA);

    gTess->SetImage(maly.data, maly.cols, maly.rows, 1, (int)maly.step);
    unique_ptr<char[]> text(gTess->GetUTF8Text());
    int conf = gTess->MeanTextConf();

    if (text && text[0] >= '1' && text[0] <= '9' && conf >= 60)
        return text[0];

    Mat th2;
    adaptiveThreshold(gray, th2, 255,
                      ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY_INV, 11, 2);
    resize(th2, maly, Size(28, 28));

    gTess->SetImage(maly.data, maly.cols, maly.rows, 1, (int)maly.step);
    unique_ptr<char[]> t2(gTess->GetUTF8Text());
    int conf2 = gTess->MeanTextConf();

    if (t2 && t2[0] >= '1' && t2[0] <= '9' && conf2 >= 60)
        return t2[0];

    Mat th3;
    threshold(gray, th3, 128, 255, THRESH_BINARY_INV);
    resize(th3, maly, Size(28, 28));

    gTess->SetImage(maly.data, maly.cols, maly.rows, 1, (int)maly.step);
    unique_ptr<char[]> t3(gTess->GetUTF8Text());
    int conf3 = gTess->MeanTextConf();

    if (t3 && t3[0] >= '1' && t3[0] <= '9' && conf3 >= 50)
        return t3[0];

    return '.';
}


class Solution {
public:
    bool solveSudoku(vector<vector<char>>& board) {
        return solve(board);
    }
private:
    bool solve(vector<vector<char>>& board) {
        for (int i = 0; i < 9; i++)
            for (int j = 0; j < 9; j++)
                if (board[i][j] == '.') {
                    for (char c = '1'; c <= '9'; c++) {
                        if (isValid(board, i, j, c)) {
                            board[i][j] = c;
                            if (solve(board)) return true;
                            board[i][j] = '.';
                        }
                    }
                    return false;
                }
        return true;
    }
    bool isValid(const vector<vector<char>>& board, int i, int j, char c) {
        for (int p = 0; p < 9; p++) {
            if (board[i][p] == c) return false;
            if (board[p][j] == c) return false;
            if (board[3*(i/3)+p/3][3*(j/3)+p%3] == c) return false;
        }
        return true;
    }
};


static bool waliduj_plansze(const vector<vector<char>>& board) {
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            char c = board[i][j];
            if (c == '.') continue;
            // Sprawdź wiersz
            for (int k = 0; k < 9; k++) {
                if (k == j) continue;
                if (board[i][k] == c) {
                    cerr << "Konflikt OCR: wiersz " << i
                         << ", kolumna " << j << " vs " << k
                         << " (" << c << ")\n";
                    return false;
                }
            }
            // Sprawdź kolumnę
            for (int k = 0; k < 9; k++) {
                if (k == i) continue;
                if (board[k][j] == c) {
                    cerr << "Konflikt OCR: kolumna " << j
                         << ", wiersz " << i << " vs " << k
                         << " (" << c << ")\n";
                    return false;
                }
            }
            // Sprawdź blok 3x3
            int bi = 3*(i/3), bj = 3*(j/3);
            for (int r = bi; r < bi+3; r++)
                for (int s = bj; s < bj+3; s++) {
                    if (r == i && s == j) continue;
                    if (board[r][s] == c) {
                        cerr << "Konflikt OCR: blok (" << i << "," << j
                             << ") vs (" << r << "," << s
                             << ") (" << c << ")\n";
                        return false;
                    }
                }
        }
    }
    return true;
}


int main(int argc, char** argv) {
    string path = "testcase1/sudoku 1.png";
    if (argc > 1) path = argv[1];

    // Wczytaj i popraw kontrast
    Mat plansza = imread(path);
    if (plansza.empty()) { cerr << "Nie można wczytać obrazu!\n"; return -1; }

    plansza = popraw_kontrast(plansza);

    if (!initTesseract()) return -1;

    Mat szary;
    cvtColor(plansza, szary, COLOR_BGR2GRAY);
    vector<Point2f> quad;
    if (!siatka_sudoku(szary, quad)) {
        cerr << "Nie znaleziono planszy sudoku!\n";
        cleanupTesseract(); return -1;
    }

    const int W = 900;
    Point2f dst[4] = { {0,0}, {(float)W,0}, {(float)W,(float)W}, {0,(float)W} };
    Mat M = getPerspectiveTransform(quad, vector<Point2f>(dst, dst + 4));
    Mat warped;
    warpPerspective(plansza, warped, M, Size(W, W), INTER_CUBIC, BORDER_REPLICATE);

    int granica = max(2, W / 250);
    Rect cropRect(granica, granica, W - 2*granica, W - 2*granica);
    warped = warped(cropRect).clone();

    const int N   = 9;
    const int kostka = warped.rows / N;
    const int pad    = max(2, (int)(kostka * 0.10));   

    vector<vector<char>> board(N, vector<char>(N, '.'));
    vector<vector<bool>> given(N, vector<bool>(N, false));

    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            int x = c * kostka + pad;
            int y = r * kostka + pad;
            int sz = max(5, kostka - 2*pad);
            Rect inner(x, y, sz, sz);
            inner &= Rect(0, 0, warped.cols, warped.rows);

#ifdef DEBUG_CELLS
            imwrite("cell_" + to_string(r) + "_" + to_string(c) + ".png", warped(inner));
#endif

            char ch = rozpoznanie_cyfry(warped(inner));
            board[r][c] = ch;
            if (ch != '.') given[r][c] = true;
        }
    }

    cout << "Wykryta plansza:\n";
    for (int r = 0; r < N; r++) {
        for (int c = 0; c < N; c++) cout << board[r][c];
        cout << "\n";
    }

    if (!waliduj_plansze(board)) {
        cerr << "OCR wykrył sprzeczne cyfry – sprawdź zdjęcie!\n";
        cleanupTesseract(); return -1;
    }

    Solution solver;
    if (!solver.solveSudoku(board)) {
        cerr << "Nie można rozwiązać sudoku – błąd OCR lub nierozwiązywalna plansza!\n";
        cleanupTesseract(); return -1;
    }

    Mat Odp = warped.clone();
    int    typ_czcionki  = FONT_HERSHEY_SIMPLEX;
    double skala_czcionki = kostka / 50.0;
    int    grubosc       = 2;

    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            if (board[r][c] != '.' && !given[r][c]) {
                string txt(1, board[r][c]);
                int tx = c * kostka + kostka / 5;
                int ty = r * kostka + 4 * kostka / 5;
                putText(Odp, txt, Point(tx, ty),
                        typ_czcionki, skala_czcionki,
                        Scalar(0, 0, 255), grubosc, LINE_AA);
            }
        }
    }

    imwrite("Rozwiazanie.png", Odp);
    cout << "\nZapisano rozwiązanie do Rozwiazanie.png\n";

    imshow("Plansza oryginalna", imread(path));
    imshow("Rozwiązanie Sudoku", Odp);
    waitKey(0);

    cleanupTesseract();
    return 0;
}