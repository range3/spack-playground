# Inherit from ./base.Dockerfile
FROM range3/spack-playground:master 

SHELL ["/bin/bash", "-l", "-c"]

RUN \
  # Common packages
  export DEBIAN_FRONTEND=noninteractive \
  && apt-get update \
  && apt-get -y install --no-install-recommends \
    libtbb-dev \
  # Spack packages
  && spack install cxxopts \
  # Clean up
  && apt-get autoremove -y \
  && apt-get clean -y \
  && rm -rf /var/lib/apt/lists/*
