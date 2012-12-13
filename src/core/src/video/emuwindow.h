/**
 * Copyright (C) 2005-2012 Gekko Emulator
 *
 * @file    emuwindow.h
 * @author  Neobrain
 * @date    2012-06-01
 * @brief   Interface for implementing an emulator window manager
 *
 * @section LICENSE
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * Official project repository can be found at:
 * http://code.google.com/p/gekko-gc-emu/
 */

#ifndef CORE_EMUWINDOW_H_
#define CORE_EMUWINDOW_H_

#include "common.h"
#include "config.h"

namespace input_common
{
class KeyboardInput;
}

// Abstraction class used to provide an interface between emulation code and the frontend (e.g. SDL, 
//  QGLWidget, GLFW, etc...)
class EmuWindow
{

public:
    /// Data structure to store an emuwindow configuration
    struct Config{
        bool    fullscreen;
        int     res_width;
        int     res_height;
    };

    /// Swap buffers to display the next frame
    virtual void SwapBuffers() = 0;

	/// Polls window events
	virtual void PollEvents() = 0;

    /// Makes the graphics context current for the caller thread
    virtual void MakeCurrent() = 0;

    /// Releases (dunno if this is the "right" word) the GLFW context from the caller thread
    virtual void DoneCurrent() = 0;

    /**
     * @brief Sets the window configuration
     * @param config Configuration to set the window to, includes fullscreen, size, etc
     */
    virtual void SetConfig(Config config) = 0;

    /**
     * @brief Sets the window title
     * @param title Title to set the window to
     */
    void SetTitle(std::string title) {
        window_title_ = title;
    }

    /**
     * @brief Gets the client area size, used by the renderer to properly scale video output
     * @param width Client area width in pixels
     * @param height Client area height in pixels
     */
    void GetClientAreaSize(int &width, int &height) {
        width = client_area_width_;
        height = client_area_height_;
    }

    /**
     * @brief Called from KeyboardInput constructor to notify EmuWindow about its presence
     * @param controller_interface Pointer to a running KeyboardInput interface
     */
    void set_controller_interface(input_common::KeyboardInput* controller_interface) { 
        controller_interface_ = controller_interface;
    }
    input_common::KeyboardInput* controller_interface() { return controller_interface_; }
    
    int client_area_width() { return client_area_width_; }
    void set_client_area_width(int val) { client_area_width_ = val; }

    int client_area_height() { return client_area_height_; }
    void set_client_area_height(int val) { client_area_height_ = val; }

protected:
    EmuWindow() : controller_interface_(NULL), client_area_width_(640), client_area_height_(480) { }
    virtual ~EmuWindow() {}

    std::string window_title_;          ///< Current window title, should be used by window impl.

    int client_area_width_;             ///< Current client width, should be set by window impl.
    int client_area_height_;            ///< Current client height, should be set by window impl.

private:
    Config config_;         ///< Internal configuration

    input_common::KeyboardInput* controller_interface_;

    DISALLOW_COPY_AND_ASSIGN(EmuWindow);
};

#endif // CORE_EMUWINDOW_H_
