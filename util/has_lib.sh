#!/usr/bin/env bash
has_lib() {
  local regex="lib$1.+(so|dylib)$"

  if [ "$1" = "jpeg" ]; then 
      # check jpeg-turbo
      ret=$(jpegtran -v 'null' 2>&1 | grep 'turbo')
      if [ "$ret" = "" ]; then
        echo 'node-images deps jpeg-turbo!'
        return 1
      fi
  fi

  # Try using ldconfig on linux systems
  for LINE in `which ldconfig 2>&1 >/dev/null && ldconfig -p 2>/dev/null | grep -E $regex`; do
    return 0
  done

  # Try just checking common library locations
  for dir in /lib /usr/lib /usr/local/lib /opt/local/lib; do
    test -d $dir && ls $dir | grep -E $regex && return 0
  done
  return 1
}

has_lib $1 > /dev/null
if test $? -eq 0; then
  echo true
else
  echo false
fi
