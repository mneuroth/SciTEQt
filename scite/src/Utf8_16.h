// @file Utf8_16.h
// Copyright (C) 2002 Scott Kirkwood
//
// Permission to use, copy, modify, distribute and sell this code
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies or
// any derived copies.  Scott Kirkwood makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.
//
// Notes: Used the UTF information I found at:
//   http://www.cl.cam.ac.uk/~mgk25/unicode.html
////////////////////////////////////////////////////////////////////////////////

#ifndef UTF8_16_H
#define UTF8_16_H

class Utf8_16 {
public:
	typedef unsigned short utf16; // 16 bits
	typedef unsigned char utf8; // 8 bits
	typedef unsigned char ubyte;
	enum encodingType {
		eUnknown,
		eUtf16BigEndian,
		eUtf16LittleEndian,  // Default on Windows
		eUtf8,
		eLast
	};
	static const utf8 k_Boms[eLast][3];
};

// Reads UTF-16 and outputs UTF-8
class Utf16_Iter : public Utf8_16 {
public:
	Utf16_Iter() noexcept;
	void set(const ubyte *pBuf, size_t nLen, encodingType eEncoding, ubyte *endSurrogate) noexcept;
	utf8 get() const noexcept {
		return m_nCur;
	}
	void operator++() noexcept;
	operator bool() const noexcept { return m_pRead <= m_pEnd; }
	utf16 read(const ubyte *pRead) const noexcept;

protected:
	enum eState {
		eStart,
		eSecondOf4Bytes,
		ePenultimate,
		eFinal
	};
protected:
	encodingType m_eEncoding;
	eState m_eState;
	utf8 m_nCur;
	int m_nCur16;
	// These 3 pointers are for externally allocated memory passed to set
	const ubyte *m_pBuf;
	const ubyte *m_pRead;
	const ubyte *m_pEnd;
};

// Reads UTF16 and outputs UTF8
class Utf8_16_Read : public Utf8_16 {
public:
	Utf8_16_Read() noexcept;

	// Deleted so Utf8_16_Read objects can not be copied.
	Utf8_16_Read(const Utf8_16_Read &) = delete;
	Utf8_16_Read(Utf8_16_Read &&) = delete;
	Utf8_16_Read &operator=(const Utf8_16_Read &) = delete;
	Utf8_16_Read &operator=(Utf8_16_Read &&) = delete;

	~Utf8_16_Read() noexcept;

	size_t convert(char *buf, size_t len);
	char *getNewBuf() noexcept { return reinterpret_cast<char *>(m_pNewBuf); }

	encodingType getEncoding() const noexcept { return m_eEncoding; }
protected:
	int determineEncoding() noexcept;
private:
	encodingType m_eEncoding;
	// m_pBuf refers to externally allocated memory
	ubyte *m_pBuf;
	// Depending on encoding, m_pNewBuf may be allocated by Utf8_16_Read::convert or
	// refer to an externally allocated block passed to convert.
	ubyte *m_pNewBuf;
	size_t m_nBufSize;
	bool m_bFirstRead;
	ubyte m_leadSurrogate[2];
	size_t m_nLen;
	Utf16_Iter m_Iter16;
};

// Read in a UTF-8 buffer and write out to UTF-16 or UTF-8
class Utf8_16_Write : public Utf8_16 {
public:
	Utf8_16_Write() noexcept;

	// Deleted so Utf8_16_Write objects can not be copied.
	Utf8_16_Write(const Utf8_16_Write &) = delete;
	Utf8_16_Write(Utf8_16_Write &&) = delete;
	Utf8_16_Write &operator=(const Utf8_16_Write &) = delete;
	Utf8_16_Write &operator=(Utf8_16_Write &&) = delete;

	~Utf8_16_Write() noexcept;

	void setEncoding(encodingType eType) noexcept;

	void setfile(FILE *pFile) noexcept;
	size_t fwrite(const void *p, size_t _size);
	int fclose() noexcept;
protected:
	encodingType m_eEncoding;
	FILE *m_pFile;
	std::unique_ptr<utf16 []>m_buf16;
	size_t m_nBufSize;
	bool m_bFirstWrite;
};

#endif
