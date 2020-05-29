//
//  XProducable.cpp
//  XExporter
//
//  Created by Oogh on 2020/3/19.
//  Copyright © 2020 Oogh. All rights reserved.
//

#include "XProducable.h"

XProducable::XProducable() {

}

XProducable::~XProducable() {

}

void XProducable::setInput(const std::string &filename) {
    mFilename = filename;
}

std::shared_ptr<XImage> XProducable::getImage(long clock) {
    return nullptr;
}

std::shared_ptr<XSample> XProducable::getSample() {
    return nullptr;
}
