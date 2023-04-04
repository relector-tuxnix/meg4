/*
 * meg4/editors/fmt_pico8.h
 *
 * Copyright (C) 2023 bzt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * @brief Parse a PICO-8 cartridge
 *
 */

/* PICO-8 codepage to UTF-8 UNICODE */
static const char *pico_utf8[] = {
    "▮","■","□","⁙","⁘","‖","◀","▶","「","」","¥","•","、","。","゛","゜",
    " ","!","\"","#","$","%","&","'","(",")","*","+",",","-",".","/",
    "0","1","2","3","4","5","6","7","8","9",":",";","<","=",">","?",
    "@","A","B","C","D","E","F","G","H","I","J","K","L","M","N","O",
    "P","Q","R","S","T","U","V","W","X","Y","Z","[","\\","]","^","_",
    "`","a","b","c","d","e","f","g","h","i","j","k","l","m","n","o",
    "p","q","r","s","t","u","v","w","x","y","z","{","|","}","~","○",
    "█","▒","🐱","⬇","️░","✽","●","♥","☉","웃","⌂","⬅","️😐","♪","🅾","️◆",
    "…","➡","️★","⧗","⬆","️ˇ","∧","❎","▤","▥","あ","い","う","え","お","か",
    "き","く","け","こ","さ","し","す","せ","そ","た","ち","つ","て","と","な","に",
    "ぬ","ね","の","は","ひ","ふ","へ","ほ","ま","み","む","め","も","や","ゆ","よ",
    "ら","り","る","れ","ろ","わ","を","ん","っ","ゃ","ゅ","ょ","ア","イ","ウ","エ",
    "オ","カ","キ","ク","ケ","コ","サ","シ","ス","セ","ソ","タ","チ","ツ","テ","ト",
    "ナ","ニ","ヌ","ネ","ノ","ハ","ヒ","フ","ヘ","ホ","マ","ミ","ム","メ","モ","ヤ",
    "ユ","ヨ","ラ","リ","ル","レ","ロ","ワ","ヲ","ン","ッ","ャ","ュ","ョ","◜","◝"
};

/* PICO-8 palette */
static uint8_t picopal[48] = {
    0x00, 0x00, 0x00, 0x1D, 0x2B, 0x53, 0x7E, 0x25, 0x53, 0x00, 0x87, 0x51, 0xAB, 0x52, 0x36, 0x5F,
    0x57, 0x4F, 0xC2, 0xC3, 0xC7, 0xFF, 0xF1, 0xE8, 0xFF, 0x00, 0x4D, 0xFF, 0xA3, 0x00, 0xFF, 0xEC,
    0x27, 0x00, 0xE4, 0x36, 0x29, 0xAD, 0xFF, 0x83, 0x76, 0x9C, 0xFF, 0x77, 0xA8, 0xFF, 0xCC, 0xAA
};
#define HEX(a) (a>='0' && a<='9' ? a-'0' : (a>='a' && a<='f' ? a-'a'+10 : (a>='A' && a<='F' ? a-'A'+10 : 0)))

/* These extremely poor quality PICO-8 Lua code inflate routines (with minor modifications for ANSI C by bzt) are from
 * https://github.com/dansanderson/lexaloffle */

/* old compression format */
#define LITERALS 60
#define READ_VAL(val) {val = *in; in++;}
static int decompress_mini(uint8_t *in_p, uint8_t *out_p, int max_len)
{
	char *literal = "^\n 0123456789abcdefghijklmnopqrstuvwxyz!#%(){}[]<>+=/*:;.,~_";
	int block_offset;
	int block_length;
	int val;
	uint8_t *in = in_p;
	uint8_t *out = out_p;
	int len;

	/* header tag ":c:" */
	READ_VAL(val);
	READ_VAL(val);
	READ_VAL(val);
	READ_VAL(val);

	/* uncompressed length */
	READ_VAL(val);
	len = val * 256;
	READ_VAL(val);
	len += val;

	/* compressed length (to do: use to check) */
	READ_VAL(val);
	READ_VAL(val);

	memset(out_p, 0, max_len);

	if (len > max_len) return 1; /* corrupt data */

	while (out < out_p + len)
	{
		READ_VAL(val);

		if (val < LITERALS)
		{
			/* literal */
			if (val == 0)
			{
				READ_VAL(val);
				/*printf("rare literal: %d\n", val);*/
				*out = val;
			}
			else
			{
				/* printf("common literal: %d (%c)\n", literal[val], literal[val]);*/
				*out = literal[val];
			}
			out++;
		}
		else
		{
			/* block */
			block_offset = val - LITERALS;
			block_offset *= 16;
			READ_VAL(val);
			block_offset += val % 16;
			block_length = (val / 16) + 2;

			memcpy(out, out - block_offset, block_length);
			out += block_length;
		}
	}
	return out - out_p;
}
#undef LITERALS

/* new compression format */
#define PXA_MIN_BLOCK_LEN 3
#define BLOCK_LEN_CHAIN_BITS 3
#define BLOCK_DIST_BITS 5
#define TINY_LITERAL_BITS 4
#define PXA_READ_VAL(x)  getval(8)
static int bit = 1;
static int byte = 0;
static int src_pos = 0;
static uint8_t *src_buf = NULL;
static int getbit(void)
{
	int ret;

	ret = (src_buf[src_pos] & bit) ? 1 : 0;
	bit <<= 1;
	if (bit == 256)
	{
		bit = 1;
		src_pos ++;
	}
	return ret;
}
static int getval(int bits)
{
	int i;
	int val = 0;
	if (bits == 0) return 0;

	for (i = 0; i < bits; i++)
		if (getbit())
			val |= (1 << i);

	return val;
}
static int getchain(int link_bits, int max_bits)
{
	int max_link_val = (1 << link_bits) - 1;
	int val = 0;
	int vv = max_link_val;
	int bits_read = 0;

	while (vv == max_link_val)
	{
		vv = getval(link_bits);
		bits_read += link_bits;
		val += vv;
		if (bits_read >= max_bits) return val; /* next val is implicitly 0 */
	}

	return val;
}
static int getnum(void)
{
	int jump = BLOCK_DIST_BITS;
	int bits = jump;
	int val;

	/* 1  15 bits // more frequent so put first */
	/* 01 10 bits */
	/* 00  5 bits */
	bits = (3 - getchain(1, 2)) * BLOCK_DIST_BITS;

	val = getval(bits);

	if (val == 0 && bits == 10)
		return -1; /* raw block marker */

	return val;
}

static int pxa_decompress(uint8_t *in_p, uint8_t *out_p, int max_len)
{
	int i;
	int literal[256];
	int literal_pos[256];
	int dest_pos = 0;
	int header[8];
	int raw_len, comp_len, block_type, block_offset, block_len, lpos, bits, safety, c;

	bit = 1;
	byte = 0;
	src_buf = in_p;
	src_pos = 0;

	for (i = 0; i < 256; i++)
		literal[i] = i;

	for (i = 0; i < 256; i++)
		literal_pos[literal[i]] = i;

	/* header */

	for (i = 0; i < 8; i++)
		header[i] = PXA_READ_VAL();

	raw_len  = header[4] * 256 + header[5];
	comp_len = header[6] * 256 + header[7];

	/* printf(" read raw_len:  %d\n", raw_len); */
	/* printf(" read comp_len: %d\n", comp_len); */

	while (src_pos < comp_len && dest_pos < raw_len && dest_pos < max_len)
	{
		block_type = getbit();

		/* printf("%d %d\n", src_pos, block_type); fflush(stdout); */

		if (block_type == 0)
		{
			/* block */

			block_offset = getnum() + 1;

			if (block_offset == 0)
			{
				/* 0.2.0j: raw block */
				while (dest_pos < raw_len)
				{
					out_p[dest_pos] = getval(8);
					if (out_p[dest_pos] == 0) /* found end -- don't advance dest_pos */
						break;
					dest_pos ++;
				}
			}
			else
			{
				block_len = getchain(BLOCK_LEN_CHAIN_BITS, 100000) + PXA_MIN_BLOCK_LEN;

				/* copy // don't just memcpy because might be copying self for repeating pattern */
				while (block_len > 0){
					out_p[dest_pos] = out_p[dest_pos - block_offset];
					dest_pos++;
					block_len--;
				}

				/* safety: null terminator. to do: just do at end */
				if (dest_pos < max_len-1)
					out_p[dest_pos] = 0;
			}
		}else
		{
			/* literal */

			lpos = 0;
			bits = 0;

			safety = 0;
			while (getbit() == 1 && safety++ < 16)
			{
				lpos += (1 << (TINY_LITERAL_BITS + bits));
				bits ++;
			}

			bits += TINY_LITERAL_BITS;
			lpos += getval(bits);

			if (lpos > 255) return 0; /* something wrong */

			/* grab character and write */
			c = literal[lpos];

			out_p[dest_pos] = c;
			dest_pos++;
			out_p[dest_pos] = 0;

			for (i = lpos; i > 0; i--)
			{
				literal[i] = literal[i-1];
				literal_pos[literal[i]] ++;
			}
			literal[0] = c;
			literal_pos[c] = 0;
		}
	}


	return 0;
}

static int is_compressed_format_header(uint8_t *dat)
{
	if (dat[0] == ':' && dat[1] == 'c' && dat[2] == ':' && dat[3] == 0) return 1;
	if (dat[0] == 0 && dat[1] == 'p' && dat[2] == 'x' && dat[3] == 'a') return 2;
	return 0;
}

/* max_len should be 0x10000 (64k max code size) */
/* out_p should allocate 0x10001 (includes null terminator) */
static int pico8_code_section_decompress(uint8_t *in_p, uint8_t *out_p, int max_len)
{
	if (is_compressed_format_header(in_p) == 0) { memcpy(out_p, in_p, 0x3d00); out_p[0x3d00] = '\0'; return 0; } /* legacy: no header -> is raw text */
	if (is_compressed_format_header(in_p) == 1) return decompress_mini(in_p, out_p, max_len);
	if (is_compressed_format_header(in_p) == 2) return pxa_decompress (in_p, out_p, max_len);
	return 0;
}

/**
 * Import a PICO-8 file
 */
static int format_pico8(uint8_t *buf, int len)
{
    uint8_t *ptr;
    int i, j;

    /* load palette */
    memset(meg4.mmio.palette, 0, sizeof(meg4.mmio.palette));
    for(i = 0; i < 16; i++)
        meg4.mmio.palette[i] = (0xff << 24) | (picopal[i * 3 + 2] << 16) | (picopal[i * 3 + 1] << 8) | picopal[i * 3 + 0];
    if(!buf) return 0;

    /* generate waveforms (same as PICO-8's, just much better quality) */
    memset(meg4.waveforms, 0, sizeof(meg4.waveforms));
    for(i = 0; i < 8; i++)
        dsp_genwave(i + 1, i);

    if(len > 16 && !memcmp(buf, "pico-8 cartridge", 16)) {
        /****** decode textual format ******/
        /* skip over header */
        for(; *buf && (buf[0] != '_' || buf[1] != '_'); buf++);
        if(!buf[0]) return 0;
        while(*buf) {

            /*** lua script ***/
            if(!memcmp(buf, "__lua__", 7)) {
                for(buf += 7; *buf == '\r' || *buf == '\n'; buf++);
                for(ptr = buf; *ptr && memcmp(ptr - 1, "\n__", 3); ptr++);
                meg4.src_len = ptr - buf + 7; memset(meg4.src_bm, 0, sizeof(meg4.src_bm));
                meg4.src = (char*)realloc(meg4.src, meg4.src_len + 1);
                if(!meg4.src) { meg4.src_len = 0; return 0; }
                memcpy(meg4.src, "#!lua\n\n", 7);
                memcpy(meg4.src + 7, buf, meg4.src_len);
                meg4.src[meg4.src_len++] = 0;
                buf = ptr;
            } else

            /*** sprites ***/
            if(!memcmp(buf, "__gfx__", 7)) {
                memset(meg4.mmio.sprites, 0, sizeof(meg4.mmio.sprites));
                for(buf += 7; *buf == '\r' || *buf == '\n'; buf++);
                /* one large 128 x 128 x 4 bit sheet, with 8 x 8 pixel sprites */
                for(i = 0; i < 8192 && *buf && *buf != '_'; i++, buf++) {
                    while(*buf == '\r' || *buf == '\n') buf++;
                    if(*buf == '_' || buf[1] == '_') break;
                    j = i >> 7; j = ((j & 8) << 4) | ((((j >> 1) & ~7) | (j & 7)) << 8) | (i & 0x7f);
                    meg4.mmio.sprites[j] = HEX(buf[0]);
                }
            } else

            /*** map ***/
            if(!memcmp(buf, "__map__", 7)) {
                memset(meg4.mmio.map, 0, sizeof(meg4.mmio.map));
                for(buf += 7; *buf == '\r' || *buf == '\n'; buf++);
                for(i = 0; i < 4096 && *buf && *buf != '_'; i++, buf += 2) {
                    while(*buf == '\r' || *buf == '\n') buf++;
                    if(*buf == '_' || buf[1] == '_') break;
                    /* 128 x 32 sprites, 8 bit per map entry, each a sprite id, big endian */
                    meg4.mmio.map[(i >> 7) * 320 + (i & 0x7f)] = (HEX(buf[0]) << 4) | HEX(buf[1]);
                }
            } else

            /* TODO: sounds and music */
            /*** sound effects ***/
            if(!memcmp(buf, "__sfx__", 7)) {
                for(buf += 7; *buf == '\r' || *buf == '\n'; buf++);
            } else

            /*** music ***/
            if(!memcmp(buf, "__music__", 9)) {
                for(buf += 9; *buf == '\r' || *buf == '\n'; buf++);
            }

            /* go to next chunk */
            while(*buf && *buf != '_') buf++;
        }
    } else {
        /****** decode binary format ******/
        if(len < 0x8000) return 0;

        /*** sprites ***/
        memset(meg4.mmio.sprites, 0, sizeof(meg4.mmio.sprites));
        for(i = 0, ptr = buf; i < 16384; i += 2, ptr++) {
            j = i >> 7; j = ((j & 8) << 4) | ((((j >> 1) & ~7) | (j & 7)) << 8) | (i & 0x7f);
            meg4.mmio.sprites[j] = *ptr & 0xf;
            meg4.mmio.sprites[j+1] = (*ptr) >> 4;
        }

        /*** map ***/
        memset(meg4.mmio.map, 0, sizeof(meg4.mmio.map));
        for(i = 0, ptr = buf + 8192; i < 4096; i++, ptr++)
            meg4.mmio.map[(i >> 7) * 320 + (i & 0x7f)] = *ptr;
        for(ptr = buf + 4096; i < 8192; i++, ptr++)
            meg4.mmio.map[(i >> 7) * 320 + (i & 0x7f)] = *ptr;

        /* TODO: sounds and music */
        /*** sound effects ***/
        /* buf + 0x3200. Format: 64 samples, each 68 bytes: 32 x 2 byte notes, 1 byte flags, 1 byte speed, 1 byte start, 1 byte end */

        /*** music ***/
        /* buf + 0x3100, 256 bytes. Format: 64 tracks, each 4 bytes */
        /* one track:
         *  byte 0: bit 7: begin loop
         *          bit 6: channel enabled
         *          bit 0..5: sound effect id
         *  byte 1: bit 7: end loop
         *          bit 6: channel enabled
         *          bit 0..5: sound effect id
         *  byte 2: bit 7: stop at end
         *          bit 6: channel enabled
         *          bit 0..5: sound effect id
         *  byte 3: bit 7: ???
         *          bit 6: channel enabled
         *          bit 0..5: sound effect id */

        /*** lua script ***/
        ptr = (uint8_t*)malloc(131072);
        if(!ptr) return 0;
        memset(ptr, 0, 131072);
        pico8_code_section_decompress(buf + 0x4300, ptr, 131072);
        meg4.src = (char*)realloc(meg4.src, 262144 + 8);
        if(!meg4.src) { free(ptr); meg4.src_len = 0; return 0; }
        meg4.src_len = 7; memset(meg4.src_bm, 0, sizeof(meg4.src_bm));
        memcpy(meg4.src, "#!lua\n\n", 7);
        for(i = 0; ptr[i] && i < 131072 && meg4.src_len < 262144 + 8; i++) {
            if(ptr[i] < 16) meg4.src[meg4.src_len++] = ptr[i];
            else {
                j = strlen(pico_utf8[(int)ptr[i] - 16]);
                memcpy(meg4.src + meg4.src_len, pico_utf8[(int)ptr[i] - 16], j);
                meg4.src_len += j;
            }
        }
        meg4.src[meg4.src_len++] = 0;
        meg4.src = (char*)realloc(meg4.src, meg4.src_len);
        free(ptr);
    }
    return 1;
}
