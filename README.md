# spack-playground

## Development
```bash
cd /workspaces/spack-playground
spack env activate -d spack/envs/dev
spack install --keep-stage
```

## activate IntelliSense provided by clangd
- the vsode extensions are already installed in the dev container.
- open vscode command palette
  - `> clangd: Download language server`
  - `> Developper: Reload Window`

## Create new spack env if you want
```bash
cd /workspaces/spack-playground
spack env create -d spack/envs/dev2
spack env activate -d spack/envs/dev2
spack compiler find
spack external find
# edit spack/envs/dev2/spack.yaml
## suggestion: remove openssl and python from external packages
spack concretize -f
spack install --keep-stage
```

## 3rd Party Library License
### Akka (https://github.com/akka/akka)

<details><summary>Apache 2 license</summary>

```
This software is licensed under the Apache 2 license, quoted below.

Copyright 2009-2018 Lightbend Inc. <https://www.lightbend.com>

Licensed under the Apache License, Version 2.0 (the "License"); you may not
use this file except in compliance with the License. You may obtain a copy of
the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
License for the specific language governing permissions and limitations under
the License.
```

</details>
