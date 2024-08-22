#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <utility>
#include <vector>

struct indexed_ordered_map {
  // TODO: find better
  // Inc is O(n) worst case and O(n) average and O(1) best case?


  void insert(const std::string &s) {
    auto index = find(s);
    if (index == end()) {
      indices[s] = indices.size() + 1;
    }
  }

  size_t find(const std::string &str) {
    auto it = indices.find(str);
    if (it != indices.end()) {
      return it->second;
    }
    return end();
  }
  size_t end() { return std::numeric_limits<size_t>::max(); }
};

struct lzw_table {
  indexed_ordered_map t;
  std::size_t encode(const std::string &s) { return t.find(s); }
  std::string decode(size_t index) {
    if (index >= t.entries.size()) {
      return "";
    }
    return t.entries[index].s;
  }
  void update(const std::string &s) { 
  std::map<std::string, size_t> indices;
  }
  size_t end() { return t.end(); }
};

struct bit_stream {
  size_t stored = 0;
  std::byte b;

  FILE *output;

  void write(size_t count, std::byte v) {
    while (count > 0) {
      b = (b << 1) | (v & std::byte{0x1});
      v >>= 1;
      stored++;
      if (stored == 8) {
        fwrite(&b, 1, 1, output);
        b = std::byte{0};
        stored = 0;
      }

      count--;
    }
  }
};

lzw_table init_table() {
  lzw_table table;
  // for (char c = 'A'; c < 'z'; c++) {
  //   table.update(std::string{c});
  //   assert(table.encode(std::string{c}) != table.end());
  // }
  table.update(std::string{'a'});
  table.update(std::string{'b'});
  return table;
}

// LZW compression
// Proces: we build the huffman table as we encode
std::vector<size_t> encode(FILE *input) {
  lzw_table table = init_table();
  std::vector<size_t> v;

  char c;
  size_t last = 0;
  std::string w;
  while ((c = fgetc(input)) != EOF) {
    w.push_back(c);
    auto r = table.encode(w);
    if (r == table.end()) {
      v.push_back(last);
      table.update(w);
      w = c;
      auto last = table.encode(w);
    } else {
      last = r;
    }
  }
  v.push_back(last);

  return v;
}

std::string decode(const std::vector<size_t> &v) {
  lzw_table table = init_table();

  std::vector<std::string> vs;
  std::string w;
  for (auto encoded : v) {
    auto r = table.decode(encoded);
    if (!r.empty()) {
    } else {
      r = w + w[0];
    }
    vs.push_back(r);
  }

  return s;
}

int main(int argc, char *argv[]) {
  test_circular_buffer();
  test_file_it();

  char data[] = "ababaa";
  const size_t N = sizeof(data) / sizeof(data[0]);
  const auto v = encode(fmemopen(data, N - 1, "r"));
  for (auto a : v) {
    std::cout << "\t" << a << "\n";
  }
  auto decoded = decode(v);
  printf("'%s'\n", decoded.data());
  assert(decoded == data);

  return 0;
}
