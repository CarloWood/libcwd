#ifndef RAW_WRITE_INL
#define RAW_WRITE_INL

#if CWDEBUG_DEBUG

namespace libcwd {

inline
_private_::raw_write_nt const&
operator<<(_private_::raw_write_nt const& raw_write, char const* data)
{
  size_t __attribute__((unused)) __libcwd_len = ::write(2, data, strlen(data));
  return raw_write;
}

inline
_private_::raw_write_nt const&
operator<<(_private_::raw_write_nt const& raw_write, void const* data)
{
  size_t dat = (size_t)data;
  size_t __attribute__((unused)) __libcwd_len = ::write(2, "0x", 2);
  char c[11];
  char* p = &c[11];
  do
  {
    int d = (dat % 16);
    *--p = ((d < 10) ? '0' : ('a' - 10)) + d;
    dat /= 16;
  }
  while(dat > 0);
  __libcwd_len = ::write(2, p, &c[11] - p);
  return raw_write;
}

inline _private_::raw_write_nt const&
operator<<(_private_::raw_write_nt const& raw_write, bool data)
{
  size_t __attribute__((unused)) __libcwd_len;
  if (data)
    __libcwd_len = ::write(2, "true", 4);
  else
    __libcwd_len = ::write(2, "false", 5);
  return raw_write;
}

inline _private_::raw_write_nt const&
operator<<(_private_::raw_write_nt const& raw_write, char data)
{
  char c[1];
  c[0] = data;
  size_t __attribute__((unused)) __libcwd_len = ::write(2, c, 1);
  return raw_write;
}

inline _private_::raw_write_nt const&
operator<<(_private_::raw_write_nt const& raw_write, unsigned long data)
{
  char c[11];
  char* p = &c[11];
  do
  {
    *--p = '0' + (data % 10);
    data /= 10;
  }
  while(data > 0);
  size_t __attribute__((unused)) __libcwd_len = ::write(2, p, &c[11] - p);
  return raw_write;
}

inline
_private_::raw_write_nt const&
operator<<(_private_::raw_write_nt const& raw_write, long data)
{
  if (data < 0)
  {
    size_t __attribute__((unused)) __libcwd_len = ::write(2, "-", 1);
    data = -data;
  }
  return operator<<(raw_write, (unsigned long)data);
}

inline _private_::raw_write_nt const&
operator<<(_private_::raw_write_nt const& raw_write, int data)
{
  return operator<<(raw_write, (long)data);
}

inline
_private_::raw_write_nt const&
operator<<(_private_::raw_write_nt const& raw_write, unsigned int data)
{
  return operator<<(raw_write, static_cast<unsigned long>(data));
}

} // namespace libcwd
#endif // CWDEBUG_DEBUG

#endif // RAW_WRITE_INL
