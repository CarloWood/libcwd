#! /bin/bash

# Update LD_LIBRARY_PATH.
export LD_LIBRARY_PATH="${1}:${LD_LIBRARY_PATH}"

# Loop through shared targets.
for i in $2; do
  echo -n "$i: "

  # Capture stderr in a variable.
  ERR_OUTPUT=$(./$i 2>&1 >/dev/null)

  if [ $? -eq 0 ]; then
    echo -e "\e[32mOK\e[0m"
  else
    if [ "$ERR_OUTPUT" = "FATAL   : Expected Failure." ]; then
      echo -e "\e[33mExpected failure\e[0m"
    else
      echo -e "\e[31mERROR\e[0m"
    fi
  fi
done

