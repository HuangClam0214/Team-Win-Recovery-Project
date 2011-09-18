// button.cpp - GUIButton object

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>

#include <string>

extern "C" {
#include "../common.h"
#include "../minui/minui.h"
#include "../recovery_ui.h"
}

#include "rapidxml.hpp"
#include "objects.hpp"

GUIButton::GUIButton(xml_node<>* node)
{
    xml_attribute<>* attr;
    xml_node<>* child;

    mButtonImg = NULL;
    mButtonIcon = NULL;
    mButtonLabel = NULL;
    mAction = NULL;

    if (!node)  return;

    // Three of the four can be loaded directly from the node
    mButtonImg = new GUIImage(node);
    mButtonLabel = new GUIText(node);
    mAction = new GUIAction(node);

    if (mButtonImg->Render() < 0)
    {
        LOGE("Unable to locate button image\n");
        delete mButtonImg;
        mButtonImg = NULL;
    }
    if (mButtonLabel->Render() < 0)
    {
        delete mButtonLabel;
        mButtonLabel = NULL;
    }

    // The icon is a special case
    child = node->first_node("icon");
    if (child)
    {
        attr = child->first_attribute("resource");
        if (attr)
            mButtonIcon = PageManager::FindResource(attr->value());
    }

    int x, y, w, h;
    if (mButtonImg)     mButtonImg->GetRenderPos(x, y, w, h);
    SetRenderPos(x, y, w, h);
    return;
}

GUIButton::~GUIButton()
{
    if (mButtonImg)     delete mButtonImg;
    if (mButtonLabel)   delete mButtonLabel;
    if (mAction)        delete mAction;
}

int GUIButton::Render(void)
{
    int ret = 0;

    if (mButtonImg)     ret = mButtonImg->Render();
    if (ret < 0)        return ret;
    if (mButtonIcon && mButtonIcon->GetResource())
        gr_blit(mButtonIcon->GetResource(), 0, 0, mIconW, mIconH, mIconX, mIconY);
    if (mButtonLabel)   ret = mButtonLabel->Render();
    if (ret < 0)        return ret;
    return ret;
}

int GUIButton::Update(void)
{
    int ret = 0, ret2 = 0;

    if (mButtonImg)         ret = mButtonImg->Update();
    if (ret < 0)            return ret;

    if (ret == 0)
    {
        if (mButtonLabel)   ret2 = mButtonLabel->Update();
        if (ret2 < 0)       return ret2;
        if (ret2 > ret)     ret = ret2;
    }
    else if (ret == 1)
    {
        // The button re-rendered, so everyone else is a render
        if (mButtonIcon && mButtonIcon->GetResource())
            gr_blit(mButtonIcon->GetResource(), 0, 0, mIconW, mIconH, mIconX, mIconY);
        if (mButtonLabel)   ret = mButtonLabel->Render();
        if (ret < 0)        return ret;
        ret = 1;
    }
    else
    {
        // Aparently, the button needs a background update
        ret = 2;
    }
    return ret;
}

int GUIButton::SetRenderPos(int x, int y, int w, int h)
{
    mRenderX = x;
    mRenderY = y;
    if (w || h)
    {
        mRenderW = w;
        mRenderH = h;
    }

    mIconW = 0;     mIconH = 0;
    if (mButtonIcon && mButtonIcon->GetResource())
    {
        mIconW = gr_get_width(mButtonIcon->GetResource());
        mIconH = gr_get_height(mButtonIcon->GetResource());
    }

    mTextH = 0;
    mTextW = 0;
    mIconX = mRenderX + ((mRenderW - mIconW) / 2);
    if (mButtonLabel)   mButtonLabel->GetCurrentBounds(mTextW, mTextH);
    if (mTextW)
    {
        mTextX = mRenderX + ((mRenderW - mTextW) / 2);
    }

    if (mIconH == 0 || mTextH == 0 || mIconH + mTextH > mRenderH)
    {
        mIconY = mRenderY + (mRenderH / 2) - (mIconH / 2);
        mTextY = mRenderY + (mRenderH / 2) - (mTextH / 2);
    }
    else
    {
        int divisor = mRenderH - (mIconH + mTextH);
        mIconY = mRenderY + (divisor / 3);
        mTextY = mRenderY + (divisor * 2 / 3) + mIconH;
    }

    if (mButtonLabel)   mButtonLabel->SetRenderPos(mTextX, mTextY);
    if (mAction)        mAction->SetActionPos(mRenderX, mRenderY, mRenderW, mRenderH);
    SetActionPos(mRenderX, mRenderY, mRenderW, mRenderH);
    return 0;
}

int GUIButton::NotifyTouch(TOUCH_STATE state, int x, int y)
{
    return (mAction ? mAction->NotifyTouch(state, x, y) : 1);
}

