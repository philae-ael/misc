package logic

import (
	"slices"
)

// MAGIC STUFF DO NOT TOUCH (or every code will be wrong... or worse yet... THE PERMUTATION WILL NOT PERMUTE)

// Some more details:
// This uses Feistel ciphers make a "pseudorandom" permutation
// https://xkcd.com/221/
// The chosen function is fnv-1 on the byte stream [x1, x2, k1, k2]
// This allow us to make a pseudo random permutation of 0..2^32
// Using a trick we make it a permutation of 0..n
// For this to work, we need n <= 2^32 and n as close as possible to 2^32

var keys = []uint16{
	20842, 51523,
}

func f(x uint16, k uint16) uint16 {
	// variation on fnv-1
	h := uint64(0xCBF29CE484222325)
	h *= 0x100000001B3
	h ^= uint64(x & 0xFF)
	h *= 0x100000001B3
	h ^= uint64((x >> 8) & 0xFF)
	h *= 0x100000001B3
	h ^= uint64(k & 0xFF)
	h *= 0x100000001B3
	h ^= uint64((k >> 8) & 0xFF)
	// h %= 1 << 16
	return uint16(h)
}

func feistel(l uint16, r uint16, k uint16) (uint16, uint16) {
	return r, f(r, k) ^ l
}

func feistelinv(l uint16, r uint16, k uint16) (uint16, uint16) {
	return f(l, k) ^ r, l
}

func encode32(b uint32) uint32 {
	b1 := uint16(b & 0xFFFF)
	b2 := uint16(b >> 16)

	for i := range keys {
		b1, b2 = feistel(b1, b2, keys[i])
	}
	return uint32(b1) | uint32(b2)<<16
}

func decode32(b uint32) uint32 {
	b1 := uint16(b & 0xFFFF)
	b2 := uint16(b >> 16)

	for i := range keys {
		b1, b2 = feistelinv(b1, b2, keys[len(keys)-1-i])
	}
	return uint32(b1) | uint32(b2)<<16
}

var alphabet = []rune{
	'A',
	'B',
	'C',
	'D',
	'E',
	'F',
	'G',
	'H',
	'J',
	'K',
	'M',
	'N',
	'P',
	'Q',
	'R',
	'S',
	'T',
	'U',
	'V',
	'W',
	'X',
	'Y',
	'Z',
	'1',
	'2',
	'3',
	'4',
	'5',
	'6',
	'7',
	'8',
	'9',
}

// no...pow in go math lib...
// TODO: TEST THAT
func fastPow(a uint32, p uint32) uint32 {
	t := uint32(1)
	for p != 0 {
		if p%2 != 0 {
			t *= a
		}
		a *= a
		p /= 2
	}
	return t
}

// assert encode_length * log2(len(alphabet)) <= 32
// + len(alphabet)**encode_length should be as close as possible to 32
var encode_length = uint32(6)

// id should be <= n (n  := len(alphabet)**encode_length)
func Encode(id uint) string {
	n := fastPow(uint32(len(alphabet)), encode_length)

	v := uint32(id)
	for {
		v = encode32(v)
		if v < n {
			break
		}
	}

	i := 0

	s := make([]rune, encode_length)
	for i := range s {
		s[i] = alphabet[0]
	}

	for v > 0 {
		idx := v % uint32(len(alphabet))
		s[i] = alphabet[idx]
		v = v / uint32(len(alphabet))
		i += 1
	}
	return string(s)

}

// The char of s have to be in alphabet
func Decode(s string) uint32 {
	v := uint32(0)
	n := fastPow(uint32(len(alphabet)), encode_length)

	for i := range s {
		idx := slices.Index(alphabet, rune(s[len(s)-1-i]))
		v = v*uint32(len(alphabet)) + uint32(idx)
	}

	for {
		v = decode32(v)
		if v < n {
			break
		}
	}

	return v

}
