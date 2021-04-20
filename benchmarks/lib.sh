function kill_pid_if_stdin_line_matches_a_pattern() {
  local _pid=$1
  local _pattern=$2
  local _line
  local _latch=0
  while IFS= read -r _line
  do
    echo $_line
    if [ $_latch -ne 0 ]; then
      continue
    fi

    if [[ $_line = *"$_pattern"* ]]; then
      kill $_pid
      _latch=1
    fi
  done
}

function wait_until_stdout() {
  local _command=$1
  local _pattern=$2

  sleep inf &
  local _dummy_pid=$!

  stdbuf -oL $_command > >(
    kill_pid_if_stdin_line_matches_a_pattern $_dummy_pid "$_pattern"
  ) &

  set +e
  wait $_dummy_pid
  set -e
}

function wait_until_stderr() {
  local _command=$1
  local _pattern=$2

  sleep inf &
  local _dummy_pid=$!

  stdbuf -eL $_command 2> >(
    kill_pid_if_stdin_line_matches_a_pattern $_dummy_pid "$_pattern" 1>&2
  ) &

  set +e
  wait $_dummy_pid
  set -e
}

function add_prefix() {
  local _prefix=$1
  local _line
  while IFS= read -r _line
  do
    echo ${_prefix}${_line}
  done
}
