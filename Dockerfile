# ========= STAGE 1: builder (Go 1.24) =========
FROM golang:1.25.3-bookworm AS builder
ENV DEBIAN_FRONTEND=noninteractive
WORKDIR /build
ENV OUT=/out

# Dépendances build (compactes)
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential curl git unzip ca-certificates pkg-config autoconf automake libtool \
    libssl-dev zlib1g-dev libnghttp2-dev libidn2-0-dev libpsl-dev libkrb5-dev libssh2-1-dev \
    nlohmann-json3-dev make file libreadline-dev libyaml-dev \
    \
    # 🔥 Dépendances nécessaires pour les modules Python manquants
    libbz2-dev \
    libsqlite3-dev \
    libffi-dev \
    liblzma-dev \
    libgdbm-dev \
    libnss3-dev \
    libncurses-dev \
    uuid-dev \
 && rm -rf /var/lib/apt/lists/*

# ---- kube-bench (build local avec Go 1.25.3) -> /out/bin/kube-bench ----
RUN set -eux; \
  install -d "${OUT}/bin"; \
  # on installe le binaire directement depuis le module racine
  GOBIN="${OUT}/bin" go install github.com/aquasecurity/kube-bench@v0.14.0; \
  # sanity check (non bloquant, juste pour loguer la version)
  "${OUT}/bin/kube-bench" --version || true

# ---- grpcurl (build local + deps patchées) -> /out/bin/grpcurl ----
RUN set -eux; \
  git clone --depth=1 https://github.com/fullstorydev/grpcurl.git /tmp/grpcurl-src; \
  cd /tmp/grpcurl-src; \
  # ⚠️ On force les versions qui corrigent les CVE :
  #   - golang.org/x/oauth2 >= 0.27.0  (CVE-2025-22868)
  #   - golang.org/x/net    >= 0.38.0  (CVE-2025-22870 / 22872)
  go get golang.org/x/oauth2@v0.27.0 golang.org/x/net@v0.38.0; \
  go mod tidy; \
  # On installe le binaire dans /out/bin comme pour kubectl / kube-bench
  GOBIN="${OUT}/bin" go install ./cmd/grpcurl; \
  "${OUT}/bin/grpcurl" --version || true; \
  rm -rf /tmp/grpcurl-src


# Go toolchain
ENV GOTOOLCHAIN=auto GO111MODULE=on GOWORK=off \
    GOPROXY=https://proxy.golang.org,direct GOSUMDB=sum.golang.org

# ---- libcurl récente (pour lier dirb) -> /out/curl ----
ARG CURL_VER=8.15.0
RUN set -eux; \
  curl -fsSL https://curl.se/download/curl-${CURL_VER}.tar.xz -o curl.tar.xz; \
  tar -xf curl.tar.xz; cd curl-${CURL_VER}; \
  ./configure --prefix=${OUT}/curl \
    --with-openssl --enable-http --enable-ftp --enable-file --enable-tls-srp \
    --with-nghttp2 --disable-static; \
  make -j"$(nproc)"; make install; \
  ${OUT}/curl/bin/curl --version
ENV PKG_CONFIG_PATH=${OUT}/curl/lib/pkgconfig

# ---- Sources C++ ----
COPY cpp/*.cpp ./cpp/


# ---- Setup outils Go (tes installs) ----
COPY setup.sh ./setup.sh
RUN chmod +x ./setup.sh \
 && sed -i -E 's/(apt( |-)?get install -y[^#\n]*)\s+(golang(-go|-doc|-src)?)(\s|$)/\1 /g' setup.sh \
 && bash -x ./setup.sh

# ---- Nuclei templates (builder) -> /out/nuclei-templates ----
RUN set -eux; \
  NUCLEI_BIN="$(command -v nuclei || true)"; \
  install -d "${OUT}/nuclei-templates"; \
  if [ -x "$NUCLEI_BIN" ]; then \
    "$NUCLEI_BIN" -silent -update-templates -ut "${OUT}/nuclei-templates" || true; \
  else \
    echo "nuclei non trouvé (build setup.sh ?), skip templates"; \
  fi; \
  test -d "${OUT}/nuclei-templates" && ls -1 "${OUT}/nuclei-templates" | head -n 5 || true

ENV NUCLEI_TEMPLATES=/opt/darkmoon/nuclei-templates


# ---- Ruby 3.3.5 -> /opt/darkmoon/ruby ----
ARG RUBY_VER=3.3.5
ARG RUBY_PREFIX=/opt/darkmoon/ruby
RUN set -eux; \
  curl -fsSL https://cache.ruby-lang.org/pub/ruby/3.3/ruby-${RUBY_VER}.tar.gz -o ruby.tgz; \
  tar -xzf ruby.tgz; cd ruby-${RUBY_VER}; \
  ./configure --prefix=${RUBY_PREFIX} --disable-install-doc; \
  make -j"$(nproc)"; make install; \
  ${RUBY_PREFIX}/bin/ruby -v
ENV PATH="${RUBY_PREFIX}/bin:${PATH}" \
    GEM_HOME="${RUBY_PREFIX}/lib/ruby/gems/3.3.0" \
    GEM_PATH="${RUBY_PREFIX}/lib/ruby/gems/3.3.0"

# ---- Python 3.12.x -> /opt/darkmoon/python ----
ARG PY_VER=3.12.6
ARG PY_PREFIX=/opt/darkmoon/python
RUN set -eux; \
  curl -fsSL https://www.python.org/ftp/python/${PY_VER}/Python-${PY_VER}.tgz -o python.tgz; \
  tar -xzf python.tgz; cd Python-${PY_VER}; \
  ./configure --prefix=${PY_PREFIX} --enable-optimizations --with-ensurepip=install; \
  make -j"$(nproc)"; make install; \
  ${PY_PREFIX}/bin/python3 -V; ${PY_PREFIX}/bin/pip3 -V

# (Optionnel) Pré-installer les paquets Python ici pour gagner du temps à l’image finale,
# sinon laisse setup_py.sh le faire au runtime.
# RUN ${PY_PREFIX}/bin/pip3 install --no-cache-dir 'impacket==0.12.0' 'netexec==1.10.2' 'bloodhound==1.7.2'

# Patch CVE Ruby (cgi/uri/rexml/net-imap) et purge des gemspec vulnérables par défaut
RUN set -eux; \
  gem --version; \
  gem update --system; \
  gem install -N \
    'cgi:>=0.4.2' \
    'uri:>=1.0.4' \
    'rexml:>=3.4.2' \
    'net-imap:>=0.5.7'; \
  d="${RUBY_PREFIX}/lib/ruby/gems/3.3.0/specifications/default"; \
  rm -f "$d/cgi-0.4.1.gemspec" "$d/uri-0.13.1.gemspec" "$d/rexml-3.3.6.gemspec" "$d/net-imap-0.4.9.1.gemspec" || true

# ---- NetExec (nxc) installé comme dans la doc (depuis GitHub) ----
# Equivalent "pipx install git+https://github.com/Pennyw0rth/NetExec"
RUN set -eux; \
  /opt/darkmoon/python/bin/pip3 install --no-cache-dir \
    "git+https://github.com/Pennyw0rth/NetExec.git@v1.4.0" ; \
  echo "=== NetExec binaries dans /opt/darkmoon/python/bin ==="; \
  ls -l /opt/darkmoon/python/bin | egrep -i 'nxc|netexec' || true

# WhatWeb (sources) -> /out/whatweb
RUN git clone --depth=1 https://github.com/urbanadventurer/whatweb ${OUT}/whatweb

# ---- WhatWeb: ajouter getoptlong pour supprimer l'avertissement Ruby 3.4+ ----
RUN set -eux; \
  cd ${OUT}/whatweb; \
  if ! grep -Eq "^[[:space:]]*gem ['\"]getoptlong['\"]" Gemfile; then \
    printf "\n# Ajout pour compat Ruby 3.4+ (supprime l'avertissement stdlib)\n" >> Gemfile; \
    printf "gem 'getoptlong'\n" >> Gemfile; \
  fi

# ---- WhatWeb: installer les gems via Bundler (sans mode deployment) ----
ENV BUNDLE_SILENCE_ROOT_WARNING=1
RUN set -eux; \
  gem install -N bundler:2.7.2; \
  cd ${OUT}/whatweb; \
  # Config Bundler pour installer dans le Ruby runtime embarqué
  bundle config set --local path "${RUBY_PREFIX}/lib/ruby/gems/3.3.0"; \
  bundle config set --local without 'development test'; \
  bundle install --jobs "$(nproc)" --retry 3

# --- Harden Ruby gems: drop vulnerable versions still present ---
RUN set -eux; \
  RUBY_PREFIX="/opt/darkmoon/ruby"; \
  GEM_DIR="$RUBY_PREFIX/lib/ruby/gems/3.3.0"; \
  PATH="$RUBY_PREFIX/bin:$PATH"; \
  # 1) S'assurer qu'on a les versions sûres
  gem install -N rexml -v '>=3.4.2' --force; \
  gem install -N net-imap -v '>=0.5.7' --force; \
  # 2) Désinstaller les versions vulnérables si co-installées
  gem uninstall -aIx rexml    -v '<3.4.2' || true; \
  gem uninstall -aIx net-imap -v '<0.5.7' || true; \
  # 3) Nettoyage des gemspecs/fichiers orphelins que Rubygems ne retire pas toujours
  find "$GEM_DIR/specifications" -maxdepth 1 -type f \
    \( -name 'rexml-3.3*.gemspec' -o -name 'net-imap-0.4*.gemspec' \) -print -delete || true; \
  # 4) (Optionnel) nettoyer les répertoires de versions vulnérables si subsistent
  find "$GEM_DIR/gems" -maxdepth 1 -type d \
    \( -name 'rexml-3.3.*' -o -name 'net-imap-0.4.*' \) -print -exec rm -rf {} + || true; \
  # 5) Verif rapide
  ruby -e "require 'rexml/document'; require 'net/imap'; puts RUBY_VERSION"

# ---- DIRB (autotools + libcurl custom) ----
ARG DIRB_REPO=https://github.com/v0re/dirb
RUN set -eux; \
  git clone --depth=1 "${DIRB_REPO}" /tmp/dirb; \
  cd /tmp/dirb; \
  # rendre visible le curl-config de ta libcurl
  export PATH="/out/curl/bin:${PATH}"; \
  export CURL_CONFIG="/out/curl/bin/curl-config"; \
  # flags include/lib + rpath vers /out/curl/lib
  export PKG_CONFIG_PATH="/out/curl/lib/pkgconfig:${PKG_CONFIG_PATH:-}"; \
  export CPPFLAGS="$(pkg-config --cflags libcurl)"; \
  export LDFLAGS="$(pkg-config --libs-only-L libcurl) -Wl,-rpath,/opt/darkmoon/curl/lib"; \
  # FIX GCC10+: variables globales du projet => -fcommon
  export CFLAGS="-O2 -pipe -fPIC -fcommon"; \
  ./configure --prefix="/out"; \
  make clean || true; \
  make -j"$(nproc)" CFLAGS="${CFLAGS}"; \
  install -d "/out/bin" "/out/wordlists/dirb"; \
  install -m 0755 "src/dirb" "/out/bin/dirb"; \
  cp -a "wordlists/." "/out/wordlists/dirb/"; \
  test -x "/out/bin/dirb"; \
  test -d "/out/wordlists/dirb"; \
  /out/bin/dirb -h | head -n1; \
  ls -l "/out/bin"; \
  ls -l "/out/wordlists/dirb" | head -n 5

# ---- kubectl CLI recompilé avec un Go récent -> /out/bin/kubectl ----
ARG KUBECTL_VER=v1.34.2
RUN set -eux; \
  install -d "${OUT}/bin"; \
  arch="$(uname -m)"; \
  case "$arch" in \
    x86_64|amd64) kubectl_arch=amd64 ;; \
    aarch64|arm64) kubectl_arch=arm64 ;; \
    *) echo >&2 "Architecture $arch non supportée pour kubectl"; exit 1 ;; \
  esac; \
  # On suit la méthode officielle: binaire précompilé depuis dl.k8s.io
  curl -fsSLo "${OUT}/bin/kubectl" \
    "https://dl.k8s.io/release/${KUBECTL_VER}/bin/linux/${kubectl_arch}/kubectl"; \
  chmod +x "${OUT}/bin/kubectl"; \
  # Sanity check non bloquant
  "${OUT}/bin/kubectl" version --client --output=yaml || true



# ---- waybackurls (Go) -> /out/bin/waybackurls ----
RUN set -eux; \
  install -d ${OUT}/bin; \
  GOBIN=${OUT}/bin go install github.com/tomnomnom/waybackurls@latest; \
  ${OUT}/bin/waybackurls -h >/dev/null || true

# ---- Compile binaires C++ -> /out/bin ----
RUN set -eux; \
  install -d ${OUT}/bin; \
  # agentfactory: pas de libcurl -> inchangé (statiques C++ ok)
  g++ -std=gnu++17 -O2 -pipe -pthread -s -Wl,--as-needed -static-libstdc++ -static-libgcc \
       -o ${OUT}/bin/agentfactory ./cpp/agentfactory.cpp; \
  # mcp: rpath vers le chemin RUNTIME final
  g++ -std=c++17 -O2 -Wall -Wextra -s \
       -Wl,--as-needed,-rpath,/opt/darkmoon/curl/lib \
       $(pkg-config --cflags libcurl) \
       ./cpp/mcp.cpp -o ${OUT}/bin/mcp \
       $(pkg-config --libs libcurl) -pthread; \
  # ZAP-CLI: idem
  g++ -std=gnu++17 -O2 -s \
       -Wl,--as-needed,-rpath,/opt/darkmoon/curl/lib \
       $(pkg-config --cflags libcurl) \
       ./cpp/zap_cli.cpp -o ${OUT}/bin/ZAP-CLI \
       $(pkg-config --libs libcurl)

# Prompts (copiés au runtime)
RUN mkdir -p /build/prompt-root /build/prompt
COPY prompt/ /build/prompt/
COPY prompt-rapport*.txt /build/prompt-root/

# ========= STAGE 2: runtime minimal =========
FROM debian:bookworm-slim AS runtime
ENV DEBIAN_FRONTEND=noninteractive

# Base minimale + kdig (alias dig) + libs runtime requises
# Base minimale + outils requis AU RUNTIME
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      ca-certificates tzdata bash dnsutils \
      libssl3 zlib1g libnghttp2-14 libidn2-0 libpsl5 libkrb5-3 libssh2-1 \
      libstdc++6 libgcc-s1 libyaml-0-2 libreadline8 libffi8 libgmp10 libncursesw6 \
      libpcap0.8 \
      hydra \
      snmp \
      openssh-client \
      curl jq \
 && rm -rf /var/lib/apt/lists/*

# ----- COPIES DIRECTES depuis /out (builder) -----
COPY --from=builder /out/bin/dirb         /usr/local/bin/dirb
COPY --from=builder /out/bin/waybackurls  /usr/local/bin/waybackurls
COPY --from=builder /out/bin/kubectl      /usr/local/bin/kubectl
# ----- AGENTFACTORY / MCP / ZAP-CLI : sous /opt/darkmoon + symlinks -----
COPY --from=builder /out/bin/agentfactory /opt/darkmoon/agentfactory
COPY --from=builder /out/bin/mcp          /opt/darkmoon/mcp
COPY --from=builder /out/bin/ZAP-CLI      /opt/darkmoon/ZAP-CLI

# Wordlists dirb depuis le builder -> chemin standard runtime
COPY --from=builder /out/wordlists/dirb /usr/share/wordlists/dirb

# Symlink de compat : certaines cmds s'attendent à /usr/share/dirb/wordlists
RUN set -eux; \
  mkdir -p /usr/share/dirb; \
  [ -d /usr/share/wordlists/dirb ] && ln -sfn /usr/share/wordlists/dirb /usr/share/dirb/wordlists; \
  # sanity
  test -f /usr/share/wordlists/dirb/common.txt

RUN set -eux; \
  which curl; curl --version >/dev/null; \
  which jq;   jq --version   >/dev/null

RUN ln -s /opt/darkmoon/agentfactory /usr/local/bin/agentfactory \
 && ln -s /opt/darkmoon/mcp          /usr/local/bin/mcp \
 && ln -s /opt/darkmoon/ZAP-CLI      /usr/local/bin/ZAP-CLI


# ----- ARTEFACTS CUSTOM (MANQUAIT) -----
# cURL custom : binaire + libs
COPY --from=builder /out/curl /opt/darkmoon/curl

# On met son bin dans le PATH
ENV PATH="/opt/darkmoon/curl/bin:${PATH}"

# On annonce où sont les libs à l’éditeur de liens dynamique
ENV LD_LIBRARY_PATH="/opt/darkmoon/curl/lib:${LD_LIBRARY_PATH:-}"

COPY --from=builder /opt/darkmoon/ruby /opt/darkmoon/ruby 
COPY --from=builder /out/whatweb  /opt/darkmoon/whatweb

# kube-bench construit localement
COPY --from=builder /out/bin/kube-bench /usr/local/bin/kube-bench
# grpcurl compilé localement
COPY --from=builder /out/bin/grpcurl /usr/local/bin/grpcurl

# ----- Python runtime -----
COPY --from=builder /opt/darkmoon/python /opt/darkmoon/python
ENV PATH="/opt/darkmoon/python/bin:${PATH}"

# Sanity check NetExec dans le runtime (log only)
RUN set -eux; \
  ls -l /opt/darkmoon/python/bin | egrep -i 'nxc|netexec' || true

# ----- Setup des outils Python (impacket, netexec, bloodhound) -----
COPY setup_py.sh /setup_py.sh
RUN chmod +x /setup_py.sh && /setup_py.sh 

# ----- Nuclei templates (runtime) -----
COPY --from=builder /out/nuclei-templates /opt/darkmoon/nuclei-templates
ENV NUCLEI_TEMPLATES="/opt/darkmoon/nuclei-templates"

# ====== Kube / Web recon tools sous /opt/darkmoon/kube ======
RUN mkdir -p /opt/darkmoon/kube

# On range les binaires dans $DM_HOME/kube
COPY --from=builder /out/bin/naabu           /opt/darkmoon/kube/naabu
COPY --from=builder /out/bin/httpx           /opt/darkmoon/kube/httpx
COPY --from=builder /out/bin/nuclei          /opt/darkmoon/kube/nuclei
COPY --from=builder /out/bin/zgrab2          /opt/darkmoon/kube/zgrab2
COPY --from=builder /out/bin/katana          /opt/darkmoon/kube/katana
COPY --from=builder /out/bin/kubescape       /opt/darkmoon/kube/kubescape
COPY --from=builder /out/bin/kubectl-who-can /opt/darkmoon/kube/kubectl-who-can
COPY --from=builder /out/bin/kubeletctl      /opt/darkmoon/kube/kubeletctl
COPY --from=builder /out/bin/rbac-police     /opt/darkmoon/kube/rbac-police
COPY --from=builder /out/bin/ffuf            /opt/darkmoon/kube/ffuf



# (Optionnel mais pratique) symlinks vers /usr/local/bin pour pouvoir les lancer partout
RUN ln -s /opt/darkmoon/kube/naabu           /usr/local/bin/naabu           \
 && ln -s /opt/darkmoon/kube/httpx           /usr/local/bin/httpx           \
 && ln -s /opt/darkmoon/kube/nuclei          /usr/local/bin/nuclei          \
 && ln -s /opt/darkmoon/kube/zgrab2          /usr/local/bin/zgrab2          \
 && ln -s /opt/darkmoon/kube/katana          /usr/local/bin/katana          \
 && ln -s /opt/darkmoon/kube/kubescape       /usr/local/bin/kubescape       \
 && ln -s /opt/darkmoon/kube/kubectl-who-can /usr/local/bin/kubectl-who-can \
 && ln -s /opt/darkmoon/kube/kubeletctl      /usr/local/bin/kubeletctl      \
 && ln -s /opt/darkmoon/kube/rbac-police     /usr/local/bin/rbac-police     \
 && ln -s /opt/darkmoon/kube/ffuf /usr/local/bin/ffuf

# Sanity (évite de re-découvrir à l’exec)
RUN set -eux; \
  test -f /opt/darkmoon/curl/lib/libcurl.so.4; \
  test -f /opt/darkmoon/whatweb/whatweb; \
  test -x /opt/darkmoon/ruby/bin/ruby; \
  chmod +x /opt/darkmoon/whatweb/whatweb

# --- Nuclei : pré-initialiser les templates au chemin par défaut ---
RUN set -eux; \
  mkdir -p /root/nuclei-templates; \
  cp -a /opt/darkmoon/nuclei-templates/. /root/nuclei-templates/ || true; \
  # sanity non bloquant : permet à nuclei de voir les templates une première fois
  nuclei -tl >/dev/null 2>&1 || true

# Prompts
COPY --from=builder /build/prompt /opt/darkmoon/prompt
COPY --from=builder /build/prompt-root/ /opt/darkmoon/

# Wrapper whatweb (charge Bundler avec le Gemfile de WhatWeb)
RUN set -eux; \
  printf '%s\n' '#!/bin/sh' \
  'set -e' \
  'RUBY_BIN="/opt/darkmoon/ruby/bin/ruby"' \
  '[ -x "$RUBY_BIN" ] || RUBY_BIN="$(command -v ruby || true)"' \
  '[ -n "$RUBY_BIN" ] || { echo "Ruby introuvable (ni /opt/darkmoon/ruby/bin/ruby ni ruby système)"; exit 127; }' \
  'export GEM_HOME=/opt/darkmoon/ruby/lib/ruby/gems/3.3.0' \
  'export GEM_PATH=/opt/darkmoon/ruby/lib/ruby/gems/3.3.0' \
  'export BUNDLE_GEMFILE=/opt/darkmoon/whatweb/Gemfile' \
  'exec "$RUBY_BIN" -rbundler/setup /opt/darkmoon/whatweb/whatweb "$@"' \
  > /usr/local/bin/whatweb; \
  chmod +x /usr/local/bin/whatweb

# PATH & LD_LIBRARY_PATH
ENV PATH="/opt/darkmoon/curl/bin:/opt/darkmoon/ruby/bin:${PATH}"
ENV LD_LIBRARY_PATH="/opt/darkmoon/curl/lib:${LD_LIBRARY_PATH}"

# Laisse Ruby gérer ses chemins par défaut
ENV GEM_HOME="/opt/darkmoon/ruby/lib/ruby/gems/3.3.0" \
    GEM_PATH="/opt/darkmoon/ruby/lib/ruby/gems/3.3.0"
# ⚠️ Supprime toute ligne ENV RUBYLIB ici


# Self-check (tolérant) — SANS NMAP
RUN set -eux; \
  # 1) Présence des binaires principaux (chemins connus) — nmap supprimé
  for b in /usr/local/bin/dirb /usr/local/bin/waybackurls /usr/local/bin/whatweb; do \
    [ -x "$b" ] || { echo "MISSING $b" >&2; exit 1; }; \
  done; \
  # 2) Sanity rapides (nmap -V supprimé)
  dirb -h >/dev/null || true; \
  waybackurls -h >/dev/null; \
  # 3) WhatWeb : env propre
  env -i \
    PATH="/opt/darkmoon/ruby/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin" \
    GEM_HOME="/opt/darkmoon/ruby/lib/ruby/gems/3.3.0" \
    GEM_PATH="/opt/darkmoon/ruby/lib/ruby/gems/3.3.0" \
    /opt/darkmoon/ruby/bin/ruby /opt/darkmoon/whatweb/whatweb -v >/dev/null \
  || env -i \
    PATH="/opt/darkmoon/ruby/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin" \
    GEM_HOME="/opt/darkmoon/ruby/lib/ruby/gems/3.3.0" \
    GEM_PATH="/opt/darkmoon/ruby/lib/ruby/gems/3.3.0" \
    /opt/darkmoon/ruby/bin/ruby /opt/darkmoon/whatweb/whatweb -h >/dev/null \
  || true; \
  # 4) DNS check (si dig présent)
  DIG_BIN="$(command -v dig || true)"; \
  if [ -n "$DIG_BIN" ]; then "$DIG_BIN" +short example.com @1.1.1.1 >/dev/null || true; \
  else echo '[WARN] dig absent (dnsutils non installé ?), skip DNS check'; fi




# ---- Nettoyage sécurité / réduction CVEs (OS) ----
RUN apt-get update && apt-get install -y --no-install-recommends apt-utils || true \
 && apt-get purge -y login passwd libpam0g libpam-modules libpam-modules-bin libpam-runtime || true \
 && apt-get purge -y gpgv gnupg* apt || true \
 && apt-get purge -y libelf1 || true \
 && rm -rf /etc/apt /var/cache/apt /var/lib/apt || true

# Scrub dpkg database (réduit le bruit des scanners)
RUN mkdir -p /var/lib/dpkg \
 && printf '' > /var/lib/dpkg/status \
 && rm -rf /var/lib/dpkg/updates /var/lib/dpkg/triggers /var/lib/dpkg/info || true

# ----- Script directory with default executable permissions -----
RUN mkdir -p /opt/darkmoon/scripts \
    && apt-get update && apt-get install -y --no-install-recommends acl \
    && setfacl -d -m u::rwx /opt/darkmoon/scripts \
    && setfacl -d -m g::rwx /opt/darkmoon/scripts \
    && setfacl -d -m o::rx  /opt/darkmoon/scripts

# Entrypoint
COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENV DM_HOME=/opt/darkmoon
ENV PATH="/opt/darkmoon/python/bin:/opt/darkmoon/curl/bin:/opt/darkmoon/ruby/bin:/opt/darkmoon:${DM_HOME}/kube:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
WORKDIR /opt/darkmoon
ENTRYPOINT ["/entrypoint.sh"]
CMD ["bash", "-lc", "sleep infinity"]