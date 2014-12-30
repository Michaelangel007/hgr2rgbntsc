/*
    Copyleft {C} 2014 Michael Pohoreski
    License: GPL2
    Description: command line utility to convert an 8K Apple .hgr to .tga (or .bmp)
        https://github.com/Michaelangel007/hgr2rgbntsc
    Notes: Extracted from AppleWin NTSC
        https://github.com/AppleWin/AppleWin/tree/AppleWin-Sheldon/source

MSVC2010 Debug:
    Configuration Properties, Debugging
        Command Arguments: hgr/archon.hgr2
        Working Directory: $(ProjectDir)
*/

// Includes _______________________________________________________________
    #include "StdAfx.h"
    #include "wsvideo.cpp"
    #include "cs.cpp"

// Defines ________________________________________________________________

    #if defined(_MSC_VER)
        #define PACKED
        #pragma pack(push,1)
    #endif
    #if defined(__GNUC__)
        #define PACKED __attribute__ ((__packed__))
    #endif

// Types __________________________________________________________________

    enum
    {
         _8K =  8 * 1024,
        _64K = 64 * 1024
    };

    enum TargaImageType_e
    {
        TARGA_TYPE_RGB = 2
    };

    struct TargaHeader_t
    {                                // Addr Bytes
        uint8_t nIdBytes             ; // 00 01 size of ID field that follows 18 byte header (0 usually)
        uint8_t bHasPalette          ; // 01 01
        uint8_t iImageType           ; // 02 01 type of image 0=none,1=indexed,2=rgb,3=grey,+8=rle packed

        int16_t iPaletteFirstColor   ; // 03 02
        int16_t nPaletteColors       ; // 05 02
        uint8_t nPaletteBitsPerEntry ; // 07 01 number of bits per palette entry 15,16,24,32

        int16_t nOriginX             ; // 08 02 image x origin
        int16_t nOriginY             ; // 0A 02 image y origin
        int16_t nWidthPixels         ; // 0C 02
        int16_t nHeightPixels        ; // 0E 02
        uint8_t nBitsPerPixel        ; // 10 01 image bits per pixel 8,16,24,32
        uint8_t iDescriptor          ; // 11 01 image descriptor bits (vh flip bits)

        // pixel data...
        //uint8_t aPixelData[1]        ; // 12 ?? variable length RGB data
    } PACKED;

    enum BitmapImageType_e
    {
        BMP_TYPE_RGB = 0 // WinGDI.h BI_RGB
    };

    struct bgra_t
    {
        uint8_t b;
        uint8_t g;
        uint8_t r;
        uint8_t a; // reserved on Win32
    };

    struct WinBmpHeader_t
    {
        // BITMAPFILEHEADER     // Addr Size
        uint8_t nCookie[2]      ; // 0x00 0x02 BM
        int32_t nSizeFile       ; // 0x02 0x04 0 = ignore
        int16_t nReserved1      ; // 0x06 0x02
        int16_t nReserved2      ; // 0x08 0x02
        int32_t nOffsetData     ; // 0x0A 0x04
        //                      ==      0x0D (14)

        // BITMAPINFOHEADER
        int32_t nStructSize     ; // 0x0E 0x04 biSize
        int32_t nWidthPixels    ; // 0x12 0x04 biWidth
        int32_t nHeightPixels   ; // 0x16 0x04 biHeight
        int16_t nPlanes         ; // 0x1A 0x02 biPlanes
        int16_t nBitsPerPixel   ; // 0x1C 0x02 biBitCount
        int32_t nCompression    ; // 0x1E 0x04 biCompression 0 = BI_RGB
        int32_t nSizeImage      ; // 0x22 0x04 0 = ignore
        int32_t nXPelsPerMeter  ; // 0x26 0x04
        int32_t nYPelsPerMeter  ; // 0x2A 0x04
        int32_t nPaletteColors  ; // 0x2E 0x04
        int32_t nImportantColors; // 0x32 0x04
        //                      ==      0x28 (40)

        // RGBQUAD
        // pixelmap
    } PACKED;

#ifdef _MSC_VER
    #pragma pack(pop)
#endif // _MSC_VER

    enum VideoFlag_e
    {
        VF_80COL  = 0x00000001,
        VF_DHIRES = 0x00000002,
        VF_HIRES  = 0x00000004,
        VF_MASK2  = 0x00000008,
        VF_MIXED  = 0x00000010,
        VF_PAGE2  = 0x00000020,
        VF_TEXT   = 0x00000040
    };

// Globals (Private ) _____________________________________________________
    uint8_t  gaMemMain[ _64K ];
    uint8_t  gaMemAux [ _64K ];
    uint32_t gaFrameBuffer[ FRAMEBUFFER_H ][ FRAMEBUFFER_W ];

    int      g_bVideoMode;

    char     BAD_TARGA__HEADER_SIZE_Compiler_Packing_not_18[ sizeof( TargaHeader_t  ) ==  18       ];
    char     BAD_BITMAP_HEADER_SIZE_Compiler_Packing_not_54[ sizeof( WinBmpHeader_t ) == (14 + 40) ];
    bool     g_bOutputBMP = false;
    bool     g_bScanLines50Percent = false; // leave every other line in the output blank

// Prototypes _____________________________________________________________
    void convert( const char *pSrcFileName );
    void init_mem();
    void init_videomode();
    void hgr2rgb();
    void rgb2bmp( FILE *pDstFile );
    void rgb2tga( FILE *pDstFile );
    void rgb32to24write( FILE *pDstFile );
    int usage();


// Implementation _________________________________________________________

    //========================================================================
    void convert( const char *pSrcFileName )
    {
        char   aDstFileName[ 256 ];
        char  *pDstFileName = aDstFileName;
        size_t nLen = strlen( pSrcFileName );

        strcpy( aDstFileName       , pSrcFileName );
        strcat( pDstFileName + nLen, g_bOutputBMP ? ".bmp" : ".tga" );

printf( "Src: '%s'\n", pSrcFileName );
printf( "Dst: '%s'\n", pDstFileName ); 

        FILE *pSrcFile = fopen( pSrcFileName, "rb" );
        FILE *pDstFile = fopen( pDstFileName, "w+b" );

        if( pSrcFile )
        {
            size_t nPageHGR = 0x2000;
            fread( gaMemMain + nPageHGR, _8K, 1, pSrcFile );

            hgr2rgb();

            if( g_bOutputBMP )
                rgb2bmp( pDstFile );
            else
                rgb2tga( pDstFile );

        } 
        else
            printf( "ERROR: Couldn't open source file: '%s'\n", pSrcFileName );

        fclose( pDstFile );
        fclose( pSrcFile );
    }

    //========================================================================
    void hgr2rgb()
    {
        g_pFuncVideoUpdate( VIDEO_SCANNER_6502_CYCLES );
    }

    //========================================================================
    void init_mem()
    {
        memset( gaMemMain, 0, sizeof( gaMemMain ) );
        memset( gaMemMain, 0, sizeof( gaMemAux  ) );

        memset( gaFrameBuffer, 0, sizeof( gaFrameBuffer ) );
    }

    void init_videomode()
    {
        // From Video.cpp VideoSetMode()
        wsTextPage = 1;
        wsHiresPage = 1;
        if (g_bVideoMode & VF_PAGE2) {
            if (0 == (g_bVideoMode & VF_MASK2)) {
                wsTextPage  = 2;
                wsHiresPage = 2;
            }
        }

        if (g_bVideoMode & VF_TEXT) {
            if (g_bVideoMode & VF_80COL)
                g_pFuncVideoUpdate = wsUpdateVideoText80;
            else
                g_pFuncVideoUpdate = wsUpdateVideoText40;
        }
        else if (g_bVideoMode & VF_HIRES) {
            if (g_bVideoMode & VF_DHIRES)
                if (g_bVideoMode & VF_80COL)
                    g_pFuncVideoUpdate = wsUpdateVideoDblHires;
                else
                    g_pFuncVideoUpdate = wsUpdateVideoHires0;
            else
              g_pFuncVideoUpdate = wsUpdateVideoHires;
        }
        else {
            if (g_bVideoMode & VF_DHIRES)
                if (g_bVideoMode & VF_80COL)
                    g_pFuncVideoUpdate = wsUpdateVideoDblLores;
                else
                    g_pFuncVideoUpdate = wsUpdateVideo7MLores;
            else
              g_pFuncVideoUpdate = wsUpdateVideoLores;
        }
    }

    uint8_t *MemGetMainPtr( uint16_t address )
    {
        return &gaMemMain[ address ];
    }

    uint8_t *MemGetAuxPtr ( uint16_t address )
    {
        return &gaMemAux[ address ];
    }

    //========================================================================
    void rgb2bmp( FILE *pDstFile )
    {
        WinBmpHeader_t bmp, *pBMP = &bmp;
        
        pBMP->nCookie[ 0 ] = 'B'; // 0x42
        pBMP->nCookie[ 1 ] = 'M'; // 0x4d
        pBMP->nSizeFile  = 0;
        pBMP->nReserved1 = 0;
        pBMP->nReserved2 = 0;
        pBMP->nOffsetData = sizeof(WinBmpHeader_t);
        pBMP->nStructSize = 0x28; // sizeof( WinBmpHeader_t );
        pBMP->nWidthPixels = FRAMEBUFFER_W;
        pBMP->nHeightPixels = FRAMEBUFFER_H;
        pBMP->nPlanes        = 1;
        pBMP->nBitsPerPixel  = 24;
        pBMP->nCompression   = BMP_TYPE_RGB; // BI_RGB;
        pBMP->nSizeImage     = 0;
        pBMP->nXPelsPerMeter = 0;
        pBMP->nYPelsPerMeter = 0;
        pBMP->nPaletteColors = 0;
        pBMP->nImportantColors = 0;
        fwrite( (void*)&bmp, sizeof( WinBmpHeader_t ), 1, pDstFile );
        rgb32to24write( pDstFile );
    }

    //========================================================================
    void rgb2tga( FILE *pDstFile )
    {
        TargaHeader_t tga, *pTargaHeader = &tga;
        memset( (void*)pTargaHeader, 0, sizeof( TargaHeader_t ) );

        pTargaHeader->iImageType    = TARGA_TYPE_RGB;
        pTargaHeader->nWidthPixels  = FRAMEBUFFER_W;
        pTargaHeader->nHeightPixels = FRAMEBUFFER_H;
        pTargaHeader->nBitsPerPixel = 24;
        fwrite( (void*)&tga          , sizeof( TargaHeader_t ), 1, pDstFile );
      //fwrite( (void*)&gaFrameBuffer, sizeof( gaFrameBuffer ), 1, pDstFile ); // See 32-bit note below
        rgb32to24write( pDstFile );
    }
    
    void rgb32to24write( FILE *pDstFile )
    {
        const int SRC_LINE_BYTES = FRAMEBUFFER_W * 4; // 32-bit
        const int DST_LINE_BYTES = FRAMEBUFFER_W * 3; // 24-bit
        uint8_t   destLine[ DST_LINE_BYTES ];

        /// Note: Framebuffer is 32-bit but we need to write 24-bit
        uint8_t *pSrc = (uint8_t*) &gaFrameBuffer[0][0];
        uint8_t *pDst;

        if( !g_bScanLines50Percent )
            pSrc += SRC_LINE_BYTES; // start on odd scanline

        for( int y = 0; y < FRAMEBUFFER_H; y++ )
        {
            pDst = destLine;

            for( int x = 0; x < FRAMEBUFFER_W; x++ )
            {
                *pDst++ = *pSrc++; // b
                *pDst++ = *pSrc++; // g
                *pDst++ = *pSrc++; // r
                /*     */  pSrc++; // a skip
            }
            fwrite( (void*)&destLine, DST_LINE_BYTES, 1, pDstFile );

            if( !g_bScanLines50Percent )
            {
                fwrite( (void*)&destLine, DST_LINE_BYTES, 1, pDstFile );
                y++;
                pSrc += SRC_LINE_BYTES; // skip odd source scanlines
            }
        }
    }

    //========================================================================
    int usage()
    {
        printf(
"hgr2rgb, Version 1 by Michael Pohoreski\n"
"usage: [-bmp | -tga] filename\n"
"Convert filename to .tga (default)\n"
"Source code and examples can be found at:\n"
"    https://github.com/Michaelangel007/hgr2rgbntsc\n"
        );
        return 0;
    }

    //========================================================================
    int main( const int nArg, const char *aArg[] )
    {
        init_mem();

        // From: Video.cpp wsVideoCreateDIBSection()
        uint8_t *g_pFramebufferbits = (uint8_t*) &gaFrameBuffer[0][0];
        for (int y = 0; y < 384; y++)
            wsLines[y] = g_pFramebufferbits + 4 * FRAMEBUFFER_W * ((FRAMEBUFFER_H - 1) - y - 18) + 80;

        wsVideoInitModel( 1 ); // Apple //e
        wsVideoInit();
        wsVideoStyle( 1, 2 ); // 1=single pixel, 2=double pixel

        g_bVideoMode = VF_HIRES;
        init_videomode();

        if( nArg > 1 )
        {
            int iArg = 1;
            if( strcmp( aArg[1], "-bmp" ) == 0 )
            {
                g_bOutputBMP = true;
                iArg = 2;
            }
            else
            if( strcmp( aArg[1], "-tga" ) == 0 )
            {
                g_bOutputBMP = false;
                iArg = 2;
            }
            if( strcmp( aArg[1], "-?" ) == 0 )
                return usage();

            convert( aArg[ iArg ] );
        }
        else
            usage();

        return 0;
    }

