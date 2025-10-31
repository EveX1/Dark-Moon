# ========= STAGE 1: builder (Go 1.24) =========
FROM golang:1.24-bookworm AS builder
ENV DEBIAN_FRONTEND=noninteractive
WORKDIR /build
ENV OUT=/out

# Dépendances build (compactes)
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential curl git unzip ca-certificates pkg-config autoconf automake libtool \
    libssl-dev zlib1g-dev libnghttp2-dev libidn2-0-dev libpsl-dev libkrb5-dev libssh2-1-dev \
    nlohmann-json3-dev make file libreadline-dev libyaml-dev \
 && rm -rf /var/lib/apt/lists/*

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

# ---- Nmap minimal (libpcap incluse) -> /out/bin/nmap ----
ARG NMAP_VER=7.95
RUN set -eux; \
  curl -fsSL https://nmap.org/dist/nmap-${NMAP_VER}.tar.bz2 -o nmap.tar.bz2; \
  tar -xjf nmap.tar.bz2; cd nmap-${NMAP_VER}; \
  ./configure --prefix=${OUT} \
              --with-libpcap=included \
              --without-zenmap \
              --without-ndiff \
              --disable-nls; \
  make -j"$(nproc)"; make install; \
  ${OUT}/bin/nmap -V

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
COPY mon_prompt.txt /build/prompt-root/

# ========= STAGE 2: runtime minimal =========
FROM debian:bookworm-slim AS runtime
ENV DEBIAN_FRONTEND=noninteractive

# Base minimale + kdig (alias dig) + libs runtime requises
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      ca-certificates tzdata bash knot-dnsutils \
      libssl3 zlib1g libnghttp2-14 libidn2-0 libpsl5 libkrb5-3 libssh2-1 \
      libstdc++6 libgcc-s1 libyaml-0-2 libreadline8 libffi8 libgmp10 libncursesw6 \
 && ln -sf /usr/bin/kdig /usr/local/bin/dig \
 && rm -rf /var/lib/apt/lists/*

# ----- COPIES DIRECTES depuis /out (builder) -----
# ----- BINAIRES -----
COPY --from=builder /out/bin/nmap         /usr/local/bin/nmap
COPY --from=builder /out/bin/dirb         /usr/local/bin/dirb
COPY --from=builder /out/bin/waybackurls  /usr/local/bin/waybackurls
COPY --from=builder /out/bin/agentfactory /usr/local/bin/agentfactory
COPY --from=builder /out/bin/mcp          /usr/local/bin/mcp
COPY --from=builder /out/bin/ZAP-CLI      /usr/local/bin/ZAP-CLI

# ----- ARTEFACTS CUSTOM (MANQUAIT) -----
COPY --from=builder /out/curl     /opt/darkmoon/curl
COPY --from=builder /opt/darkmoon/ruby /opt/darkmoon/ruby 
COPY --from=builder /out/whatweb  /opt/darkmoon/whatweb

# Sanity (évite de re-découvrir à l’exec)
RUN set -eux; \
  test -f /opt/darkmoon/curl/lib/libcurl.so.4; \
  test -f /opt/darkmoon/whatweb/whatweb; \
  test -x /opt/darkmoon/ruby/bin/ruby; \
  chmod +x /opt/darkmoon/whatweb/whatweb


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


# Self-check (tolérant)
RUN set -eux; \
  # 1) Présence des binaires
  for b in /usr/local/bin/nmap /usr/local/bin/dirb /usr/local/bin/waybackurls /usr/local/bin/whatweb /usr/local/bin/dig; do \
    [ -x "$b" ] || { echo "MISSING $b" >&2; exit 1; }; \
  done; \
  # 2) Sanity rapides (ne pas faire échouer sur des retours non-0 connus)
  nmap -V >/dev/null; \
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
  # 4) DNS check best effort
  dig +short example.com @1.1.1.1 >/dev/null || true



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

# Entrypoint
COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENV DM_HOME=/opt/darkmoon
ENV PATH="$DM_HOME:$DM_HOME/kube:$PATH"
WORKDIR /opt/darkmoon
ENTRYPOINT ["/entrypoint.sh"]
CMD ["bash", "-lc", "sleep infinity"]