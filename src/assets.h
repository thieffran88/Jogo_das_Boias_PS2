/* Gerado por tools/gen_assets.py - nao edite a mao. */
#ifndef ASSETS_H
#define ASSETS_H

#define ASSET_PAGE_SIZE 512
#define ASSET_NUM_PAGES 4
#define ASSET_BIN_SIZE 921600
#define WATER_FRAMES 10
#define YFONT_H 36

typedef struct { unsigned int idx_off, pal_off, h; } AssetPage;
typedef struct { unsigned char page; unsigned short x, y, w, h; } AssetSprite;

enum {
    SPR_LOGO,
    SPR_CHAR_P1,
    SPR_CHAR_P2,
    SPR_CHAR_P3,
    SPR_CHAR_P4,
    SPR_BADGE_P1,
    SPR_BADGE_P2,
    SPR_BADGE_P3,
    SPR_BADGE_P4,
    SPR_R_P1_IDLE,
    SPR_R_P1_HAPPY,
    SPR_R_P1_SAD,
    SPR_R_P2_IDLE,
    SPR_R_P2_HAPPY,
    SPR_R_P2_SAD,
    SPR_R_P3_IDLE,
    SPR_R_P3_HAPPY,
    SPR_R_P3_SAD,
    SPR_R_P4_IDLE,
    SPR_R_P4_HAPPY,
    SPR_R_P4_SAD,
    SPR_VICTORY,
    SPR_DEFEAT,
    SPR_VICTORY_BASE,
    SPR_DEFEAT_BASE,
    SPR_DIE_1,
    SPR_DIE_2,
    SPR_DIE_3,
    SPR_DIE_4,
    SPR_DIE_5,
    SPR_DIE_6,
    SPR_RING_RED,
    SPR_RING_YELLOW,
    SPR_RING_BLUE,
    SPR_RING_GREEN,
    SPR_ROPE,
    SPR_LIFEGUARD,
    SPR_BTN_JOGAR,
    SPR_BTN_CONTINUAR,
    SPR_BTN_OPCOES,
    SPR_BTN_VOLTAR,
    SPR_BTN_CREDITOS,
    SPR_BTN_SAIR,
    SPR_BTN_COMOJOGAR,
    SPR_BTN_BLANK_YELLOW,
    SPR_BTN_BLANK_BLUE,
    SPR_BTN_BLANK_RED,
    SPR_ICON_X,
    SPR_ICON_O,
    SPR_ICON_TRI,
    SPR_ICON_SQ,
    SPR_ARROW_L,
    SPR_ARROW_R,
    SPR_ARROW_U,
    SPR_ARROW_D,
    SPR_STAR,
    SPR_TROPHY,
    SPR_BURST,
    SPR_FX_EXCLAIM,
    SPR_FX_HEART,
    SPR_CLOUD_1,
    SPR_CLOUD_2,
    SPR_BUSH_1,
    SPR_BUSH_2,
    SPR_PALM,
    SPR_UMBRELLA,
    SPR_BUNTING,
    SPR_BUOY_POST,
    SPR_WATER_0,
    SPR_WATER_1,
    SPR_WATER_2,
    SPR_WATER_3,
    SPR_WATER_4,
    SPR_WATER_5,
    SPR_WATER_6,
    SPR_WATER_7,
    SPR_WATER_8,
    SPR_WATER_9,
    SPR_YF_65,
    SPR_YF_66,
    SPR_YF_67,
    SPR_YF_68,
    SPR_YF_69,
    SPR_YF_70,
    SPR_YF_71,
    SPR_YF_72,
    SPR_YF_73,
    SPR_YF_74,
    SPR_YF_75,
    SPR_YF_76,
    SPR_YF_77,
    SPR_YF_78,
    SPR_YF_79,
    SPR_YF_80,
    SPR_YF_81,
    SPR_YF_82,
    SPR_YF_83,
    SPR_YF_84,
    SPR_YF_85,
    SPR_YF_86,
    SPR_YF_87,
    SPR_YF_88,
    SPR_YF_89,
    SPR_YF_90,
    SPR_YF_48,
    SPR_YF_49,
    SPR_YF_50,
    SPR_YF_51,
    SPR_YF_52,
    SPR_YF_53,
    SPR_YF_54,
    SPR_YF_55,
    SPR_YF_56,
    SPR_YF_57,
    SPR_YF_33,
    SPR_YF_63,
    SPR_YF_58,
    SPR_YF_59,
    SPR_YF_46,
    SPR_YF_44,
    SPR_YF_43,
    SPR_YF_45,
    SPR_YF_120,
    SPR_YF_47,
    SPR_YF_61,
    SPR_COUNT
};

static const AssetPage asset_pages[ASSET_NUM_PAGES] = {
    {0, 262144, 512},
    {263168, 525312, 512},
    {526336, 657408, 256},
    {658432, 920576, 512},
};

static const AssetSprite asset_sprites[SPR_COUNT] = {
    {0, 2, 164, 330, 155}, /* LOGO */
    {0, 334, 164, 134, 118}, /* CHAR_P1 */
    {0, 371, 321, 103, 118}, /* CHAR_P2 */
    {0, 2, 321, 127, 118}, /* CHAR_P3 */
    {0, 131, 321, 123, 118}, /* CHAR_P4 */
    {1, 364, 226, 46, 64}, /* BADGE_P1 */
    {1, 412, 226, 45, 64}, /* BADGE_P2 */
    {1, 459, 226, 45, 64}, /* BADGE_P3 */
    {1, 2, 302, 45, 64}, /* BADGE_P4 */
    {0, 413, 441, 64, 70}, /* R_P1_IDLE */
    {1, 188, 122, 91, 80}, /* R_P1_HAPPY */
    {0, 90, 441, 84, 70}, /* R_P1_SAD */
    {0, 258, 441, 76, 70}, /* R_P2_IDLE */
    {1, 2, 226, 102, 74}, /* R_P2_HAPPY */
    {0, 336, 441, 75, 70}, /* R_P2_SAD */
    {0, 2, 441, 86, 70}, /* R_P3_IDLE */
    {1, 106, 226, 92, 74}, /* R_P3_HAPPY */
    {1, 333, 122, 107, 74}, /* R_P3_SAD */
    {1, 200, 226, 87, 74}, /* R_P4_IDLE */
    {1, 289, 226, 73, 72}, /* R_P4_HAPPY */
    {0, 176, 441, 80, 70}, /* R_P4_SAD */
    {0, 2, 2, 183, 160}, /* VICTORY */
    {0, 187, 2, 160, 160}, /* DEFEAT */
    {1, 2, 122, 184, 102}, /* VICTORY_BASE */
    {1, 177, 2, 162, 115}, /* DEFEAT_BASE */
    {1, 221, 302, 54, 54}, /* DIE_1 */
    {1, 277, 302, 54, 54}, /* DIE_2 */
    {1, 389, 302, 52, 54}, /* DIE_3 */
    {1, 333, 302, 54, 54}, /* DIE_4 */
    {1, 2, 368, 50, 54}, /* DIE_5 */
    {1, 443, 302, 52, 54}, /* DIE_6 */
    {2, 437, 163, 52, 35}, /* RING_RED */
    {2, 2, 201, 52, 35}, /* RING_YELLOW */
    {2, 56, 86, 52, 36}, /* RING_BLUE */
    {2, 2, 86, 52, 37}, /* RING_GREEN */
    {0, 479, 441, 14, 45}, /* ROPE */
    {1, 107, 2, 68, 118}, /* LIFEGUARD */
    {1, 165, 466, 160, 40}, /* BTN_JOGAR */
    {2, 2, 2, 159, 40}, /* BTN_CONTINUAR */
    {1, 194, 424, 165, 40}, /* BTN_OPCOES */
    {2, 163, 2, 159, 40}, /* BTN_VOLTAR */
    {1, 2, 466, 161, 40}, /* BTN_CREDITOS */
    {2, 2, 44, 155, 40}, /* BTN_SAIR */
    {1, 2, 424, 190, 40}, /* BTN_COMOJOGAR */
    {1, 327, 466, 160, 40}, /* BTN_BLANK_YELLOW */
    {2, 324, 2, 159, 40}, /* BTN_BLANK_BLUE */
    {2, 159, 44, 155, 40}, /* BTN_BLANK_RED */
    {2, 221, 201, 27, 28}, /* ICON_X */
    {2, 250, 201, 27, 28}, /* ICON_O */
    {2, 279, 201, 27, 28}, /* ICON_TRI */
    {2, 191, 201, 28, 28}, /* ICON_SQ */
    {2, 308, 201, 27, 28}, /* ARROW_L */
    {2, 160, 201, 29, 28}, /* ARROW_R */
    {2, 365, 201, 25, 28}, /* ARROW_U */
    {2, 337, 201, 26, 28}, /* ARROW_D */
    {2, 56, 201, 33, 34}, /* STAR */
    {1, 54, 368, 49, 52}, /* TROPHY */
    {1, 341, 2, 143, 106}, /* BURST */
    {2, 127, 201, 31, 30}, /* FX_EXCLAIM */
    {2, 91, 201, 34, 30}, /* FX_HEART */
    {2, 398, 44, 78, 40}, /* CLOUD_1 */
    {2, 316, 44, 80, 40}, /* CLOUD_2 */
    {1, 247, 368, 89, 46}, /* BUSH_1 */
    {1, 105, 368, 140, 46}, /* BUSH_2 */
    {0, 256, 321, 113, 118}, /* PALM */
    {1, 2, 2, 103, 118}, /* UMBRELLA */
    {1, 49, 302, 170, 60}, /* BUNTING */
    {1, 281, 122, 50, 80}, /* BUOY_POST */
    {3, 0, 0, 256, 102}, /* WATER_0 */
    {3, 256, 0, 256, 102}, /* WATER_1 */
    {3, 0, 102, 256, 102}, /* WATER_2 */
    {3, 256, 102, 256, 102}, /* WATER_3 */
    {3, 0, 204, 256, 102}, /* WATER_4 */
    {3, 256, 204, 256, 102}, /* WATER_5 */
    {3, 0, 306, 256, 102}, /* WATER_6 */
    {3, 256, 306, 256, 102}, /* WATER_7 */
    {3, 0, 408, 256, 102}, /* WATER_8 */
    {3, 256, 408, 256, 102}, /* WATER_9 */
    {2, 188, 86, 33, 36}, /* YF_65 */
    {2, 460, 86, 30, 36}, /* YF_66 */
    {2, 2, 125, 30, 36}, /* YF_67 */
    {2, 258, 86, 32, 36}, /* YF_68 */
    {2, 34, 125, 30, 36}, /* YF_69 */
    {2, 192, 125, 28, 36}, /* YF_70 */
    {2, 66, 125, 30, 36}, /* YF_71 */
    {2, 223, 86, 33, 36}, /* YF_72 */
    {2, 331, 163, 19, 36}, /* YF_73 */
    {2, 98, 125, 30, 36}, /* YF_74 */
    {2, 292, 86, 32, 36}, /* YF_75 */
    {2, 402, 125, 27, 36}, /* YF_76 */
    {2, 110, 86, 38, 36}, /* YF_77 */
    {2, 326, 86, 32, 36}, /* YF_78 */
    {2, 394, 86, 31, 36}, /* YF_79 */
    {2, 222, 125, 28, 36}, /* YF_80 */
    {2, 427, 86, 31, 36}, /* YF_81 */
    {2, 252, 125, 28, 36}, /* YF_82 */
    {2, 282, 125, 28, 36}, /* YF_83 */
    {2, 312, 125, 28, 36}, /* YF_84 */
    {2, 342, 125, 28, 36}, /* YF_85 */
    {2, 372, 125, 28, 36}, /* YF_86 */
    {2, 150, 86, 36, 36}, /* YF_87 */
    {2, 360, 86, 32, 36}, /* YF_88 */
    {2, 130, 125, 29, 36}, /* YF_89 */
    {2, 161, 125, 29, 36}, /* YF_90 */
    {2, 431, 125, 27, 36}, /* YF_48 */
    {2, 352, 163, 18, 36}, /* YF_49 */
    {2, 171, 163, 25, 36}, /* YF_50 */
    {2, 198, 163, 25, 36}, /* YF_51 */
    {2, 460, 125, 27, 36}, /* YF_52 */
    {2, 225, 163, 25, 36}, /* YF_53 */
    {2, 252, 163, 25, 36}, /* YF_54 */
    {2, 279, 163, 24, 36}, /* YF_55 */
    {2, 31, 163, 26, 36}, /* YF_56 */
    {2, 2, 163, 27, 36}, /* YF_57 */
    {2, 372, 163, 18, 36}, /* YF_33 */
    {2, 305, 163, 24, 36}, /* YF_63 */
    {2, 392, 163, 14, 36}, /* YF_58 */
    {0, 495, 441, 15, 36}, /* YF_59 */
    {2, 408, 163, 13, 36}, /* YF_46 */
    {2, 423, 163, 12, 36}, /* YF_44 */
    {2, 59, 163, 26, 36}, /* YF_43 */
    {2, 87, 163, 26, 36}, /* YF_45 */
    {1, 489, 466, 21, 36}, /* YF_120 */
    {2, 115, 163, 26, 36}, /* YF_47 */
    {2, 143, 163, 26, 36}, /* YF_61 */
};

static const short yfont_map[128] = {
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, SPR_YF_33, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, SPR_YF_43, SPR_YF_44, SPR_YF_45, SPR_YF_46, SPR_YF_47,
    SPR_YF_48, SPR_YF_49, SPR_YF_50, SPR_YF_51, SPR_YF_52, SPR_YF_53, SPR_YF_54, SPR_YF_55,
    SPR_YF_56, SPR_YF_57, SPR_YF_58, SPR_YF_59, -1, SPR_YF_61, -1, SPR_YF_63,
    -1, SPR_YF_65, SPR_YF_66, SPR_YF_67, SPR_YF_68, SPR_YF_69, SPR_YF_70, SPR_YF_71,
    SPR_YF_72, SPR_YF_73, SPR_YF_74, SPR_YF_75, SPR_YF_76, SPR_YF_77, SPR_YF_78, SPR_YF_79,
    SPR_YF_80, SPR_YF_81, SPR_YF_82, SPR_YF_83, SPR_YF_84, SPR_YF_85, SPR_YF_86, SPR_YF_87,
    SPR_YF_88, SPR_YF_89, SPR_YF_90, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    SPR_YF_120, -1, -1, -1, -1, -1, -1, -1,
};

extern const unsigned char assets_bin[];

#endif
