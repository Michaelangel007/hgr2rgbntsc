/*
    Copyleft {C} 2014 Michael Pohoreski
    License: GPL2
*/

// Includes _______________________________________________________________
    #include "StdAfx.h"

//    uint8_t  csbits[ 1024 ];
//    void     make_csbits() {};
    uint8_t *MemGetMainPtr( uint16_t address );
    uint8_t *MemGetAuxPtr ( uint16_t address );

#if 1
    #include "wsvideo.cpp"
    #include "cs.cpp"
#else
    #define FRAMEBUFFER_W 600
    #define FRAMEBUFFER_H 420
#endif

// Types __________________________________________________________________

    enum
    {
         _8K =  8 * 1024,
        _64K = 64 * 1024
    };

    enum TargaImageType_e
    {
        TARGA_RGB = 2,
        TARGA_HEADER_SIZE = 0x12
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
        uint8_t aPixelData[1]        ; // 12 ?? variable length RGB data
    };

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

// Prototypes _____________________________________________________________
    void convert( const char *pSrcFileName );
    void init_mem();
    void init_videomode();
    void hgr2rgb();
    void rgb2tga( TargaHeader_t *pTargaHeader );


// Implementation _________________________________________________________

    //========================================================================
    void convert( const char *pSrcFileName )
    {
        char   aDstFileName[ 256 ];
        char  *pDstFileName = aDstFileName;
        size_t nLen = strlen( pSrcFileName );

        strcpy( aDstFileName       , pSrcFileName );
        strcat( pDstFileName + nLen, ".tga" );

printf( "Src: '%s'\n", pSrcFileName );
printf( "Dst: '%s'\n", pDstFileName ); 

        FILE *pSrcFile = fopen( pSrcFileName, "rb" );
        FILE *pDstFile = fopen( pDstFileName, "w+b" );

        size_t nPageHGR = 0x2000;
        fread( gaMemMain + nPageHGR, _8K, 1, pSrcFile );

        TargaHeader_t tga;
        hgr2rgb();
        rgb2tga( &tga );

        fwrite( (void*)&tga          , TARGA_HEADER_SIZE      , 1, pDstFile );
        fwrite( (void*)&gaFrameBuffer, sizeof( gaFrameBuffer ), 1, pDstFile );

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
    void rgb2tga( TargaHeader_t *pTargaHeader )
    {
        memset( (void*)pTargaHeader, 0, sizeof( TargaHeader_t ) );

        pTargaHeader->iImageType    = TARGA_RGB;
        pTargaHeader->nWidthPixels  = FRAMEBUFFER_W;
        pTargaHeader->nHeightPixels = FRAMEBUFFER_H;
        pTargaHeader->nBitsPerPixel = 24;
    }

    //========================================================================
    int main( const int nArg, const char *aArg[] )
    {
        init_mem();
        wsVideoInitModel( 1 ); // Apple //e
        wsVideoInit();
        wsVideoStyle( 1, 1 );

        g_bVideoMode = VF_HIRES;
        init_videomode();

        // From: Video.cpp wsVideoCreateDIBSection()
        uint8_t *g_pFramebufferbits = (uint8_t*) &gaFrameBuffer[0][0];
        for (int y = 0; y < 384; y++)
            wsLines[y] = g_pFramebufferbits + 4 * FRAMEBUFFER_W * ((FRAMEBUFFER_H - 1) - y - 18) + 80;

        if( nArg > 1 )
        {
            convert( aArg[1] );
        }

        return 0;
    }

