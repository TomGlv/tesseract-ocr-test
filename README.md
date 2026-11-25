# tesseract-ocr-test

> C++ OCR experiments: Win32 screen capture → OpenCV EAST text detection → OCR, then text sent to an LLM API (libcurl + nlohmann/json).

Prototype that led to [bot_projet_voltaire](https://github.com/TomGlv/bot_projet_voltaire).

## What it tests
- Capturing a screen region with the **Win32 GDI** API into a `cv::Mat`
- **Text detection** with the OpenCV DNN **EAST** model (`frozen_east_text_detection.pb`, not included)
- OCR of the detected regions, then sending the text to an LLM over **REST** (libcurl) and parsing the **JSON** answer

## Stack
C++17 · OpenCV (dnn, text) · libcurl · nlohmann/json · Visual Studio / vcpkg

---
Made by [Tom Galvan](https://tomglv.dev) · Bachelor in AI, ECE Paris
