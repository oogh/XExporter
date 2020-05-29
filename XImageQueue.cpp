//
//  XImageQueue.cpp
//  XExporter
//
//  Created by Oogh on 2020/3/19.
//  Copyright © 2020 Oogh. All rights reserved.
//

#include "XImageQueue.h"
#include "XImage.h"

XImageQueue::XImageQueue(int capacity)
: mCapacity(capacity), mSize(0), mWindex(0), mRindex(0), mAborted(false) {
    mImageQueue.reserve(capacity);
    for (int i = 0; i < capacity; ++i) {
        auto image = std::make_shared<XImage>();
        mImageQueue.emplace_back(image);
    }
}

XImageQueue::~XImageQueue() {
    std::lock_guard<std::mutex> lock(mMutex);
    std::vector<std::shared_ptr<XImage>>().swap(mImageQueue);
}

std::shared_ptr<XImage> XImageQueue::peekWritable() {
    {
        std::unique_lock<std::mutex> lock(mMutex);
        mCond.wait(lock, [=] {
            return mSize > 0;
        });
    }

    return mImageQueue.at(mWindex % mCapacity);
}

void XImageQueue::push() {

}

std::shared_ptr<XImage> XImageQueue::peekReadable() {
    return std::shared_ptr<XImage>();
}

void XImageQueue::next() {

}

void XImageQueue::flush() {

}

void XImageQueue::abort() {
    std::lock_guard<std::mutex> lock(mMutex);
    mAborted = true;
}
