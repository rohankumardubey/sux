/*
 * Sux: Succinct data structures
 *
 * Copyright (C) 2019 Stefano Marchini
 *
 *  This library is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as published by the Free
 *  Software Foundation; either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  This library is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "RankSelect.hpp"

namespace sux::ranking {

/**
 * Stride - Linear search on many words.
 * @bitvector: A bitvector of 64-bit words.
 * @size: The length (in words) of the bitvector.
 * @T: Underlining Fenwick tree with an ungiven <size_t> bound.
 * @WORDS: Length (in words) of the linear search stride.
 *
 */
template <template <size_t> class T, size_t WORDS> class Stride : public RankSelect {
private:
  static constexpr size_t BOUND = 64 * WORDS;
  T<BOUND> Fenwick;
  DArray<uint64_t> Vector;

public:
  Stride(uint64_t bitvector[], size_t size)
      : Fenwick(buildFenwick(bitvector, size)), Vector(DArray<uint64_t>(size)) {
    std::copy_n(bitvector, size, Vector.get());
  }

  Stride(DArray<uint64_t> bitvector, size_t size)
      : Fenwick(buildFenwick(bitvector.get(), size)), Vector(std::move(bitvector)) {}

  virtual const uint64_t *bitvector() const { return Vector.get(); }

  virtual size_t size() const { return Vector.size() * sizeof(uint64_t); }

  virtual uint64_t rank(size_t pos) const {
    size_t idx = pos / (64 * WORDS);
    uint64_t value = Fenwick.prefix(idx);

    for (size_t i = idx * WORDS; i < pos / 64; i++)
      value += nu(Vector[i]);

    return value + nu(Vector[pos / 64] & ((1ULL << (pos % 64)) - 1));
  }

  using Rank::rank;
  using Rank::rankZero;
  virtual uint64_t rank(size_t from, size_t to) const { return rank(to) - rank(from); }

  virtual size_t select(uint64_t rank) const {
    size_t idx = Fenwick.find(&rank);

    for (size_t i = idx * WORDS; i < idx * WORDS + WORDS; i++) {
      if (i >= Vector.size())
        return SIZE_MAX;

      uint64_t rank_chunk = nu(Vector[i]);
      if (rank < rank_chunk)
        return i * 64 + select64(Vector[i], rank);
      else
        rank -= rank_chunk;
    }

    return SIZE_MAX;
  }

  virtual size_t selectZero(uint64_t rank) const {
    size_t idx = Fenwick.compFind(&rank);

    for (size_t i = idx * WORDS; i < idx * WORDS + WORDS; i++) {
      if (i >= Vector.size())
        return SIZE_MAX;

      uint64_t rank_chunk = nu(~Vector[i]);
      if (rank < rank_chunk)
        return i * 64 + select64(~Vector[i], rank);
      else
        rank -= rank_chunk;
    }

    return SIZE_MAX;
  }

  virtual uint64_t update(size_t index, uint64_t word) {
    uint64_t old = Vector[index];
    Vector[index] = word;
    Fenwick.add(index / WORDS + 1, nu(word) - nu(old));

    return old;
  }

  virtual bool set(size_t index) {
    uint64_t old = Vector[index / 64];
    Vector[index / 64] |= uint64_t(1) << (index % 64);

    if (Vector[index / 64] != old) {
      Fenwick.add(index / (WORDS * 64) + 1, 1);
      return false;
    }

    return true;
  }

  virtual bool clear(size_t index) {
    uint64_t old = Vector[index / 64];
    Vector[index / 64] &= ~(uint64_t(1) << (index % 64));

    if (Vector[index / 64] != old) {
      Fenwick.add(index / (WORDS * 64) + 1, -1);
      return true;
    }

    return false;
  }

  virtual bool toggle(size_t index) {
    uint64_t old = Vector[index / 64];
    Vector[index / 64] ^= uint64_t(1) << (index % 64);
    bool was_set = Vector[index / 64] < old;
    Fenwick.add(index / (WORDS * 64) + 1, was_set ? -1 : 1);

    return was_set;
  }

  virtual size_t bitCount() const {
    return sizeof(Stride<T, WORDS>) + Vector.bitCount() - sizeof(Vector) + Fenwick.bitCount() -
           sizeof(Fenwick);
  }

private:
  static T<BOUND> buildFenwick(const uint64_t bitvector[], size_t size) {
    uint64_t *sequence = new uint64_t[size / WORDS + 1]();
    for (size_t i = 0; i < size; i++)
      sequence[i / WORDS] += nu(bitvector[i]);

    T<BOUND> tree(sequence, size / WORDS + 1);
    delete[] sequence;
    return tree;
  }

  friend std::ostream &operator<<(std::ostream &os, const Stride<T, WORDS> &bv) {
    return os << bv.Fenwick << bv.Vector;
  }

  friend std::istream &operator>>(std::istream &is, Stride<T, WORDS> &bv) {
    return is >> bv.Fenwick >> bv.Vector;
  }
};

} // namespace sux::ranking
