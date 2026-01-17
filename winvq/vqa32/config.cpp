/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/****************************************************************************
 *
 *        C O N F I D E N T I A L -- W E S T W O O D  S T U D I O S
 *
 *----------------------------------------------------------------------------
 *
 * PROJECT
 *     VQA player library. (32-Bit protected mode)
 *
 * FILE
 *     config.c
 *
 * DESCRIPTION
 *     Player configuration routines.
 *
 * PROGRAMMER
 *     Bill Randolph
 *     Denzil E. Long, Jr.
 *
 * DATE
 *     April 10, 1995
 *
 *----------------------------------------------------------------------------
 *
 * PUBLIC
 *     VQA_INIConfig     - Initialize VQAConfig structure with INI settings.
 *     VQA_DefaultConfig - Initialize VQAConfig structure with defaults.
 *
 ****************************************************************************/

#include <cstring>

#include "winvq/vqa32/vqaplay.h"
#include "winvq/vqm32/video.h"

/*---------------------------------------------------------------------------
 * PRIVATE DECLARATIONS
 *-------------------------------------------------------------------------*/

/* Default configuration structure. */
static VQAConfig _defaultconfig = {

    /* DrawerCallback: This is a function that is called for every frame
     * in the movie.
     */
    nullptr,

    /* EventHandler: This is a function that is called for every event that
     * the client requested to be notified about.
     */
    nullptr,

    /* NotifyFlags: Flags representing the events the client wishes to be
     * notified about during playback.
     */
    0,

    /* Vmode: Video mode to use. */
    MCGA,

    /* VBIBit: Vertical blank bit polarity. */
    -1,

    /* ImageBuf: Pointer to image buffer to draw into. */
    nullptr,

    /* ImageWidth, ImageHeight: Width and height dimensions of image buffer.
     * A width and height value of -1 tells the player to consider the image
     * buffer as having the same dimensions as the frames in the movie.
     */
    320,
    200, /* Image width and height */

    /* X1, Y1: These are the coordinates to put the movies frame in the image
     * buffer. Values of -1 tell the drawer to center the frames in the buffer.
     */
    -1,
    -1,

    /* FrameRate: The rate to load the frames at. A value of -1 tells the
     * player to use the framerate of the movie.
     */
    -1,

    /* DrawRate: The rate to draw the frames at. A value of -1 tells the
     * player to use the framerate of the movie. A value of 0 tells the player
     * to use a fixed rate based on the frame size.
     */
    -1,

    /* TimerMethod: Timer method to use for playback. */
    -1,

    /* DrawFlags: Various drawing related flags. */
    0,

    /* OptionFlags: Various player options. */
    VQAOPTF_AUDIO,

    /* NumFrameBufs: The number of frame buffers to allocate/use. */
    6,

    /* NumCBBufs: The number of codebook buffers to allocate/use. */
    3,

    0,        // AudioDeviceID
    nullptr,  // AudioCallback
    nullptr,  // AudioSpec

    /* VocFile: Filename of audio track override. A value of 0 tells the
     * player not to override the movies audio track.
     */
    nullptr,

    /* AudioBuf: Audio buffer to use. A value of 0 tells the player that
     * it has to allocate a buffer itself.
     */
    nullptr,

    /* AudioBufSize: Size of audio buffer to use/allocate. A value of -1
     * tells the player to compute the buffer size from the audio
     * information in the movie.
     */
    -1,

    /* AudioRate: Audio playback rate in samples per second. A value of -1
     * tells the player to use the audio rate of the movie.
     */
    -1,

    /* Volume: Volume level to playback audio track. */
    0x00FF,

    /* HMIBufSize: Size of HMIs internal buffer. */
    2048L,

    /* DigiHandle: Handle to an initialized HMI sound driver. A value of -1
     * tells the player it must initialize the HMI sound driver itself.
     */
    -1,

    /* DigiCard: HMI ID of audio card to use. A value of 0 tells the player
     * not to use any card. A value of -1 tells the player to autodetect the
     * card in the system.
     */
    -1,

    /* DigiPort: Port address of the sound card. A value of -1 tells the player
     * to autodetect this address.
     */
    -1,

    /* DigiIRQ: Interrupt number of sound card. A value of -1 tells the player
     * to autodetect the interrupt used by the card.
     */
    -1,

    /* DigiDMA: DMA channel of the sound card. A value of -1 tells the player
     * to autodetect the channel used by the card.
     */
    -1,

    /* Language: Prefered language. */
    0,

    /* CaptionFont: Caption text font. */
    nullptr,

    /* EVAFont: EVA text font. */
    nullptr,

};

/****************************************************************************
 *
 * NAME
 *     VQA_DefaultConfig - Initialize VQAConfig structure with defaults.
 *
 * SYNOPSIS
 *     VQA_DefaultConfig(Config);
 *
 *     void VQA_DefaultConfig(VQAConfig *);
 *
 * FUNCTION
 *     Initialize configuration with default settings.
 *
 * INPUTS
 *     Config - Pointer to VQA configuration structure.
 *
 * RESULT
 *     NONE
 *
 ****************************************************************************/

void VQA_DefaultConfig(VQAConfig* config) {
  memcpy(config, &_defaultconfig, sizeof(VQAConfig));
}
