spack_user_root="${HOME}/.cache/spack"

# load spack 
if [ -s "${spack_user_root}" ]; then
  . "${spack_user_root}/share/spack/setup-env.sh"
fi

# activate default spack env
if [ -n "$DOTFILES_DEFAULT_SPACK_ENV" ]; then
  if command -v spack 1>/dev/null 2>&1; then
    if spack env st | grep -q "No active environment"; then
      spack env activate "$DOTFILES_DEFAULT_SPACK_ENV"
    fi
  fi
fi
