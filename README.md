OpenCV + Tesseract OCR Sudoku Solver

A computer vision based Sudoku solver built with **C++**, **OpenCV**, and **Tesseract OCR**.
It detects a Sudoku grid from an input image, recognizes handwritten digits using OCR,
solves the puzzle, and overlays the solution back on the image.

## ✅ Features

✔️ Detection of Sudoku grid in real images  
✔️ Digit recognition with Tesseract OCR  
✔️ Automatic puzzle solving (backtracking algorithm)  
✔️ Overlay of solved digits onto the original image  
✔️ Works with rotated / low quality photos  

## 🧠 Technology Stack

- C++
- OpenCV
- Tesseract OCR
- Visual Studio 2022
- Backtracking algorithm created at leetcode

---

## 🧩 Example Testcases

### Testcase 1
Input:
![Sudoku Input 1](testcase1/sudoku-1.jpg)

Output:
![Solved Sudoku 1](Testcases/testcase1/sudoku-solution-1.jpg)

[➡️ Zobacz więcej plików testowych](Testcases/testcase1)

---

### Testcase 2
Input:
![Sudoku Input 2](Testcases/testcase2/input.jpg)

Output:
![Solved Sudoku 2](Testcases/testcase2/output.jpg)

---

## 🚀 How to Run

1. Clone the repo

2. Open the .sln in Visual Studio

3. Make sure tesseract.exe is installed and added to PATH

4. Build & Run

---

## 🧩 Algorithm

- Find contours
- Approximate to detect largest square
- Warp perspective transform
- Cell segmentation
- OCR digit extraction
- Solve via backtracking
- Overlay render

---

## 📌 TODO

- [ ] Support handwritten input
- [ ] Add GUI
- [ ] Export solution as JSON
- [ ] More robust digit cleaning

---

## Author

**Preeksik**

If you like this project, ⭐ star this repository!

