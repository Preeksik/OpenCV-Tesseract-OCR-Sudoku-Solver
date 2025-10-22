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

## 🖼️ Example Testcases

Located in `/Testcases` folder:

| Input Image | Output |
|-------------|--------|
| ![Sudoku](Testcases/sudoku1.png) | _(solution screenshot coming soon)_ |

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

