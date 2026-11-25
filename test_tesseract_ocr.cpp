#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/text/ocr.hpp>
#include <iostream>
#include <string>
#include <windows.h>
#include <chrono>
#include <thread>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Fonction pour capturer une région de l'écran
cv::Mat CaptureScreenRegion(int x, int y, int width, int height) {
    HDC hScreen = GetDC(nullptr);
    HDC hDC = CreateCompatibleDC(hScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, width, height);
    SelectObject(hDC, hBitmap);
    BitBlt(hDC, 0, 0, width, height, hScreen, x, y, SRCCOPY);

    BITMAP bmp;
    GetObject(hBitmap, sizeof(BITMAP), &bmp);

    cv::Mat mat(bmp.bmHeight, bmp.bmWidth, CV_8UC4, bmp.bmBits);
    DeleteObject(hBitmap);
    DeleteDC(hDC);
    ReleaseDC(nullptr, hScreen);

    return mat.clone();
}

// Fonction d'OCR avec OpenCV
std::string PerformOCROpenCV(const cv::Mat& image) {
    if (image.empty()) {
        std::cerr << "Erreur : image vide pour l'OCR.\n";
        return "";
    }

    // Charger le modèle EAST pour la détection
    auto net = cv::dnn::readNet("frozen_east_text_detection.pb");

    // Prétraitement de l'image
    cv::Mat blob = cv::dnn::blobFromImage(image, 1.0, cv::Size(320, 320), cv::Scalar(123.68, 116.78, 103.94), true, false);

    // Passer l'image dans le réseau
    net.setInput(blob);
    std::vector<cv::Mat> outs;
    std::vector<std::string> outNames = { "feature_fusion/Conv_7/Sigmoid", "feature_fusion/concat_3" };
    net.forward(outs, outNames);

    // TODO : Ajouter le traitement des résultats pour détecter et extraire du texte

    return "Texte détecté (détails non implémentés)";
}

// Fonction de rappel pour gérer les données reçues avec libcurl
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* out) {
    size_t totalSize = size * nmemb;
    out->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

// Envoyer le texte extrait à l'API Reverso pour correction
std::string CorrectTextWithReverso(const std::string& text) {
    CURL* curl;
    CURLcode res;
    std::string response;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://orthographe.reverso.net/api/v1/Spelling");
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        // Crée les champs de la requête POST
        std::string postFields = R"({"englishDialect":"indifferent","autoReplace":true,"getCorrectionDetails":true,"interfaceLanguage":"fr","locale":"","language":"fra","text":")" +
            text + R"(","originalText":"","spellingFeedbackOptions":{"insertFeedback":true,"userLoggedOn":false},"origin":"interactive","isHtml":false,"IsUserPremium":false})";

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "Erreur avec cURL : " << curl_easy_strerror(res) << std::endl;
            return "";
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    return response;
}

// Parser la réponse JSON de Reverso
std::string ParseReversoResponse(const std::string& response) {
    try {
        auto jsonResponse = json::parse(response);
        if (jsonResponse.contains("text")) {
            return jsonResponse["text"].get<std::string>();
        }
        else {
            return "Erreur : pas de texte corrigé trouvé.";
        }
    }
    catch (const json::exception& e) {
        return "Erreur de parsing JSON : " + std::string(e.what());
    }
}

int main() {
    std::cout << "Initialisation de l'outil d'OCR et de correction...\n";

    while (true) {
        std::cout << "Capture de l'écran...\n";

        // Capture une région de l'écran (par exemple, en haut à gauche, 300x200 pixels)
        cv::Mat image = CaptureScreenRegion(0, 0, 300, 200);

        // Effectuer l'OCR
        std::string extractedText = PerformOCROpenCV(image);
        if (extractedText.empty()) {
            std::cerr << "Aucun texte détecté.\n";
        }
        else {
            std::cout << "Texte extrait par OCR : " << extractedText << "\n";

            // Corriger le texte avec Reverso
            std::string response = CorrectTextWithReverso(extractedText);
            if (!response.empty()) {
                std::string correctedText = ParseReversoResponse(response);
                std::cout << "Texte corrigé : " << correctedText << "\n";
            }
            else {
                std::cerr << "Erreur lors de la correction.\n";
            }
        }

        std::cout << "Pause de 5 secondes avant la prochaine capture...\n";
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    return 0;
}
