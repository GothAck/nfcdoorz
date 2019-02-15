#include <iostream>
#include <streambuf>
#include <iomanip>

#pragma once

class widthbuf : public std::streambuf {
public:
  widthbuf(int w, std::streambuf *s)
    : indent_width(0),
      def_width(w),
      width(w),
      sbuf(s),
      count(0)
  {
  }

  ~widthbuf() {
    overflow('\n');
  }

  void set_indent(int w) {
    if (w == 0) {
      prefix.clear();
      indent_width = 0;
      width = def_width;
    }
    else {
      if (w < 0 && (-w) > indent_width) w = -indent_width;
      indent_width += w;
      prefix = string(indent_width, space);
      width -= w;
    }
  }

private:

  typedef std::basic_string<char_type> string;

  // This is basically a line-buffering stream buffer.
  // The algorithm is:
  // - Explicit end of line ("\r" or "\n"): we flush our buffer
  // to the underlying stream's buffer, and set our record of
  // the line length to 0.
  // - An "alert" character: sent to the underlying stream
  // without recording its length, since it doesn't normally
  // affect the a appearance of the output.
  // - tab: treated as occupying `tab_width` characters, but is
  // passed through undisturbed (but if we wanted to expand it
  // to `tab_width` spaces, that would be pretty easy to do so
  // you could adjust the tab-width if you wanted.
  // - Everything else: really basic buffering with word wrapping.
  // We try to add the character to the buffer, and if it exceeds
  // our line width, we search for the last space/tab in the
  // buffer and break the line there. If there is no space/tab,
  // we break the line at the limit.
  int_type overflow(int_type c) {
    if (traits_type::eq_int_type(traits_type::eof(), c))
      return traits_type::not_eof(c);
    switch (c) {
    case '\n':
    case '\r':
    {
      buffer += c;
      count = 0;
      sbuf->sputn(prefix.c_str(), indent_width);
      int_type rc = sbuf->sputn(buffer.c_str(), buffer.size());
      buffer.clear();
      return rc;
    }
    case '\a':
      return sbuf->sputc(c);
    case '\t':
      buffer += c;
      count += tab_width - count % tab_width;
      return c;
    default:
      if (count >= width) {
        size_t wpos = buffer.find_last_of(" \t");
        if (wpos != string::npos) {
          sbuf->sputn(prefix.c_str(), indent_width);
          sbuf->sputn(buffer.c_str(), wpos);
          count = buffer.size() - wpos - 1;
          buffer = string(buffer, wpos + 1);
        }
        else {
          sbuf->sputn(prefix.c_str(), indent_width);
          sbuf->sputn(buffer.c_str(), buffer.size());
          buffer.clear();
          count = 0;
        }
        sbuf->sputc('\n');
      }
      buffer += c;
      ++count;
      return c;
    }
  }

  size_t indent_width;
  size_t width, def_width;
  size_t count;
  size_t tab_count;
  static const int tab_width = 8;
  string prefix;

  char_type space = static_cast<char_type>(' ');

  std::streambuf *sbuf;

  string buffer;
};

class widthstream : public std::ostream {
  widthbuf buf;
public:
  widthstream(size_t width, std::ostream &os)
    : buf(width, os.rdbuf()),
      std::ostream(&buf)
  {
  }

  widthstream &indent(int w) {
    buf.set_indent(w);
    return *this;
  }
};
