# See here for image contents: https://github.com/microsoft/vscode-dev-containers/tree/v0.166.1/containers/cpp/.devcontainer/base.Dockerfile

# [Choice] Debian / Ubuntu version: debian-10, debian-9, ubuntu-20.04, ubuntu-18.04
ARG VARIANT="ubuntu-20.04"
FROM mcr.microsoft.com/vscode/devcontainers/cpp:0-${VARIANT}

ENV EDITOR vim
ENV CPM_SOURCE_CACHE ~/.cache/CPM
ENV SPACK_ROOT=/home/vscode/.cache/spack

SHELL ["/bin/bash", "-c"]

RUN \
  # Common packages
  export DEBIAN_FRONTEND=noninteractive \
  && apt-get update \
  && apt-get -y install --no-install-recommends \
    pkg-config \
    direnv \
    vim \
    bash-completion \
    clang-9 \
    clang-format \
    clang-tidy \
    clang-tools \
    iwyu \
    tree \
    file \
    environment-modules \
  # Clean up
  && apt-get autoremove -y \
  && apt-get clean -y \
  && rm -rf /var/lib/apt/lists/* \
  # direnv
  && echo 'eval "$(direnv hook bash)"' >> /etc/profile.d/10-direnv.sh

COPY spack.sh /etc/profile.d/03-spack.sh
COPY --chown=vscode:vscode packages.yaml /home/vscode/.spack/packages.yaml

USER vscode
RUN \
  echo "alias less='less -R'" >> /home/vscode/.bash_aliases \
  # spack
  && git clone https://github.com/spack/spack.git $SPACK_ROOT \
  && . /etc/profile \
  && spack compiler find
