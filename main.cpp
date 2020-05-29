#include <iostream>
#include "XExporter.h"
#include "XFileProducer.h"

void testExport() {
    std::string outPath = "/Users/andy/export.mp4";
    int width = 720;
    int height = 1280;
    int fps = 25;
    long duration = 10 * 1000;
    auto exporter = std::make_unique<XExporter>(outPath, width, height, fps, duration);
    exporter->setExportResultCallback([](ExportResult result) {
        switch (result) {
            case EXPORT_RESULT_SUCCEED: {
                std::cout << "合成结果: EXPORT_RESULT_SUCCEED" << std::endl;
            }
                break;
            case EXPORT_RESULT_CANCELED: {
                std::cout << "合成结果: EXPORT_RESULT_CANCELED" << std::endl;
            }
                break;
            case EXPORT_RESULT_FAILED: {
                std::cout << "合成结果: EXPORT_RESULT_FAILED" << std::endl;
            }
                break;
            default:
                break;
        }
    });
    exporter->setAudioDisable(true);
    exporter->start();

    auto fileProducer = std::make_unique<XFileProducer>();
    try {
        fileProducer->setInput("/Users/andy/Movies/jieqian_720x1280.rgba");
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        exit(0);
    }


    long delay = static_cast<long>(1000.0 / fps);
    int encodeCount = 0;
    for (int clock = 0; clock < duration; clock += delay) {
        auto image = fileProducer->getImage(clock);
        exporter->encodeFrame(image->pixels, image->width, image->height);
        encodeCount++;
    }
    exporter->stop();
    exporter->debug();

    std::cout << "delay: " << delay
              << "\nencodeCount: " << encodeCount
              << "\noutpath: " << outPath
              << std::endl;

    fileProducer.reset();
    exporter.reset();
}

int main() {
    getchar();
    testExport();
    getchar();
    return 0;
}
