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
	void reset() noexcept;
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
	const ubyte *m_pBuf;
	const ubyte *m_pRead;
	const ubyte *m_pEnd;
};

// Reads UTF-8 and outputs UTF-16
class Utf8_Iter : public Utf8_16 {
public:
	Utf8_Iter() noexcept;
	void reset() noexcept;
	void set(const ubyte *pBuf, size_t nLen, encodingType eEncoding);
	int get() const noexcept {
		assert(m_eState == eStart);
		return m_nCur;
	}
	bool canGet() const noexcept { return m_eState == eStart; }
	void operator++() noexcept;
	operator bool() const noexcept { return m_pRead <= m_pEnd; }

protected:
	void toStart() noexcept; // Put to start state
	enum eState {
		eStart,
		eSecondOf4Bytes,
		ePenultimate,
		eFinal
	};
protected:
	encodingType m_eEncoding;
	eState m_eState;
	int m_nCur;
	const ubyte *m_pBuf;
	const ubyte *m_pRead;
	const ubyte *m_pEnd;
};

// Reads UTF16 and outputs UTF8
class Utf8_16_Read : public Utf8_16 {
public:
	Utf8_16_Read();
	~Utf8_16_Read();

	size_t convert(char *buf, size_t len);
	char *getNewBuf() noexcept { return reinterpret_cast<char *>(m_pNewBuf); }

	encodingType getEncoding() const noexcept { return m_eEncoding; }
protected:
	int determineEncoding() noexcept;
private:
	encodingType m_eEncoding;
	ubyte *m_pBuf;
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
	Utf8_16_Write();
	~Utf8_16_Write();

	void setEncoding(encodingType eType) noexcept;

	void setfile(FILE *pFile) noexcept;
	size_t fwrite(const void *p, size_t _size);
	int fclose() noexcept;
protected:
	encodingType m_eEncoding;
	FILE *m_pFile;
	utf16 *m_pBuf;
	size_t m_nBufSize;
	bool m_bFirstWrite;
};

#endif
