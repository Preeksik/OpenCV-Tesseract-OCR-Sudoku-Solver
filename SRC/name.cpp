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
        cerr << "Błąd inicjalizacji Tesseract!" << endl;
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
    if (gTess) {
        gTess->End();
        delete gTess;
        gTess = nullptr;
    }
}
static vector<Point2f> kolejnosc_rogow(const vector<Point2f>& pts) {
    vector<Point2f> res(4);
    auto suma = [](const Point2f& p) { return p.x + p.y; };
    auto roznica = [](const Point2f& p) { return p.x - p.y; };
    int tl = 0, br = 0, tr = 0, bl = 0;
    for (int i = 1; i < 4; ++i) {
        if (suma(pts[i]) < suma(pts[tl])) tl = i;
        if (suma(pts[i]) > suma(pts[br])) br = i;
    }
    vector<int> rest;
    for (int i = 0; i < 4; ++i) if (i != tl && i != br) rest.push_back(i);
    tr = (roznica(pts[rest[0]]) > roznica(pts[rest[1]])) ? rest[0] : rest[1];
    bl = (rest[0] == tr) ? rest[1] : rest[0];
    res[0] = pts[tl];
    res[1] = pts[tr];
    res[2] = pts[br];
    res[3] = pts[bl];
    return res;
}

static bool siatka_sudoku(const Mat& imgGray, vector<Point2f>& quad) {
    Mat rozmazane, binarne;
    GaussianBlur(imgGray, rozmazane, Size(5, 5), 0);
    adaptiveThreshold(rozmazane, binarne, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY_INV, 15, 3);
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
    vector<Point> urposzcz;
    approxPolyDP(kontury[maxIdx], urposzcz, 0.02 * arcLength(kontury[maxIdx], true), true);
    vector<Point2f> corners;
    if (urposzcz.size() == 4) {
        for (auto& p : urposzcz) corners.push_back(Point2f((float)p.x, (float)p.y));
    }
    else {
        RotatedRect rr = minAreaRect(kontury[maxIdx]);
        Point2f box[4];
        rr.points(box);
        corners.assign(box, box + 4);
    }
    quad = kolejnosc_rogow(corners);
    return true;
}
static char rozpoznanie_cyfry(const Mat& cellBGR) {
    Mat gray; cvtColor(cellBGR, gray, COLOR_BGR2GRAY);
    Mat blur; GaussianBlur(gray, blur, Size(3, 3), 0);
    Mat th; threshold(blur, th, 0, 255, THRESH_BINARY_INV | THRESH_OTSU);
    double inkRatio = (double)countNonZero(th) / (th.cols * th.rows);
    if (inkRatio < 0.015) return '.';
    Mat maly; resize(th, maly, Size(28, 28), 0, 0, INTER_AREA);
    gTess->SetImage(maly.data, maly.cols, maly.rows, 1, maly.step);
    unique_ptr<char[]> text(gTess->GetUTF8Text());
    int conf = gTess->MeanTextConf();
    char out = '.';
    if (text && text[0] >= '1' && text[0] <= '9' && conf >= 60) {
        out = text[0];
    }
    else {
        adaptiveThreshold(gray, maly, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY_INV, 11, 2);
        resize(maly, maly, Size(28, 28));
        gTess->SetImage(maly.data, maly.cols, maly.rows, 1, maly.step);
        unique_ptr<char[]> t2(gTess->GetUTF8Text());
        int conf2 = gTess->MeanTextConf();
        if (t2 && t2[0] >= '1' && t2[0] <= '9' && conf2 >= 60) out = t2[0];
    }
    return out;
}
class Solution {
public:
    void solveSudoku(vector<vector<char>>& board) {
        solve(board);
    }
private:
    bool solve(vector<vector<char>>& board) {
        for (int i = 0;i < 9;i++) {
            for (int j = 0;j < 9;j++) {
                if (board[i][j] == '.') {
                    for (char c = '1';c <= '9';c++) {
                        if (isValid(board, i, j, c)) {
                            board[i][j] = c;
                            if (solve(board)) return true;
                            board[i][j] = '.';
                        }
                    }
                    return false;
                }
            }
        }
        return true;
    }
    bool isValid(vector<vector<char>>& board, int i, int j, char c) {
        for (int p = 0;p < 9;p++) {
            if (board[i][p] == c) return false;
            if (board[p][j] == c) return false;
            if (board[3 * (i / 3) + p / 3][3 * (j / 3) + p % 3] == c) return false;
        }
        return true;
    }
};
int main(){
    Mat plansza = imread("C:/Users/adria/OneDrive/Pulpit/sudoku.png");
    if (plansza.empty()) {
        cerr << "Nie można wczytać obrazu!\n"; return -1; 
    }
    if (!initTesseract()) return -1;
    Mat szary; cvtColor(plansza, szary, COLOR_BGR2GRAY);
    vector<Point2f> quad;
    if (!siatka_sudoku(szary, quad)) {
        cerr << "Nie znaleziono planszy sudoku!\n";
        cleanupTesseract(); return -1;
    }
    const int W = 900;
    Point2f dst[4] = { {0,0}, {(float)W,0}, {(float)W,(float)W}, {0,(float)W} };
    Mat M = getPerspectiveTransform(quad, vector<Point2f>(dst, dst + 4));
    Mat warped;
    warpPerspective(plansza, warped, M, Size(W, W), INTER_LINEAR, BORDER_REPLICATE);
    int granica = max(1, W / 300);
    Rect crop(granica, granica, W - 2 * granica, W - 2 * granica);
    warped = warped(crop).clone();
    const int N = 9;
    const int kostka = warped.rows / N;
    const int pad = int(kostka * 0.14);
    vector<vector<char>> board(N, vector<char>(N, '.'));
    vector<vector<bool>> given(N, vector<bool>(N, false));
    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            int x = c * kostka, y = r * kostka;
            int ix = x + pad, iy = y + pad;
            int iw = max(5, kostka - 2 * pad);
            int ih = max(5, kostka - 2 * pad);
            Rect inner(ix, iy, iw, ih);
            inner &= Rect(0, 0, warped.cols, warped.rows);

            Mat roi = warped(inner);
            char ch = rozpoznanie_cyfry(roi);
            board[r][c] = ch;
            if (ch != '.') given[r][c] = true;
        }
    }
    Solution solver;
    solver.solveSudoku(board);
    Mat Odp = warped.clone();
    int typ_czcionki = FONT_HERSHEY_SIMPLEX;
    double skala_czcionki = kostka / 50.0;
    int grubosc = 2;
    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            char ch = board[r][c];
            if (ch != '.' && !given[r][c]) {
                string txt(1, ch);
                int x = c * kostka + kostka / 5;
                int y = r * kostka + 4 * kostka / 5;
                putText(Odp, txt, Point(x, y), typ_czcionki, skala_czcionki, Scalar(0, 0, 255), grubosc, LINE_AA);
            }
        }
    }
    imwrite("Rozwiazanie.png", Odp);
    cout << "\n✅ Zapisano rozwiązanie do pliku solution.png\n";
    imshow("Plansza", plansza);
    imshow("Rozwiązanie Sudoku", Odp);  
    waitKey(0);
    cleanupTesseract();
    
    return 0;
}
