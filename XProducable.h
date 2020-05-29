//
//  XProducable.h
//  XExporter
//
//  Created by Oogh on 2020/3/19.
//  Copyright © 2020 Oogh. All rights reserved.
//

#ifndef XEXPORTER_XPRODUCABLE_H
#define XEXPORTER_XPRODUCABLE_H

#include <memory>
#include <string>

class XImage;
class XSample;

class XProducable {
public:
    XProducable();

    virtual ~XProducable();

    virtual void setInput(const std::string& filename);

    virtual std::shared_ptr<XImage> getImage(long clock);

    virtual std::shared_ptr<XSample> getSample();

protected:
    std::string mFilename;
};


#endif //XEXPORTER_XPRODUCABLE_H
