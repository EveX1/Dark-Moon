# ========= STAGE 1: builder (Go 1.24) =========
FROM golang:1.24-bookworm AS builder

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential curl git unzip ca-certificates dnsutils nmap whatweb dirb \
    libcurl4-openssl-dev nlohmann-json3-dev sudo \
 && rm -rf /var/lib/apt/lists/*

# IMPORTANT : autoriser l’auto-toolchain si besoin
ENV GOTOOLCHAIN=auto

WORKDIR /build
COPY cpp/*.cpp ./cpp/
COPY setup.sh ./setup.sh
RUN chmod +x ./setup.sh

# 🔧 Neutraliser l’installation de golang (1.19) dans setup.sh
#   On retire les paquets golang* de la ligne apt install, sans toucher le reste.
RUN sed -i -E 's/(apt( |-)?get install -y[^#\n]*)\s+(golang(-go|-doc|-src)?)(\s|$)/\1 /g' setup.sh && \
    sed -i -E 's/(apt( |-)?get install -y[^#\n]*)\s+(golang(-go|-doc|-src)?)(\s|$)/\1 /g' setup.sh

# Maintenant setup.sh utilisera la toolchain Go 1.24 de l'image
RUN ./setup.sh

# Compile les 3 binaires C++ (mêmes flags que les tiens)
# Sortie dans /build/bin
RUN mkdir -p /build/bin \
 && g++ -std=gnu++17 -O2 -pipe -pthread -o /build/bin/agentfactory ./cpp/agentfactory.cpp \
 && g++ -std=c++17 -O2 -Wall -Wextra ./cpp/mcp.cpp -o /build/bin/mcp -lcurl -pthread \
 && g++ -std=gnu++17 -O2 ./cpp/zap_cli.cpp -o /build/bin/ZAP-CLI -lcurl

# --- prompt + fichiers racine ---
RUN mkdir -p /build/prompt-root /build/prompt
COPY prompt/ /build/prompt/
COPY prompt-rapport*.txt /build/prompt-root/
COPY mon_prompt.txt /build/prompt-root/

# Rassemble tout ce qu’on veut dans l’image finale
RUN mkdir -p /artifacts \
 && mkdir -p /artifacts/kube /artifacts/prompt \
 && cp -a /build/bin/* /artifacts/ \
 && cp -a /build/kube/* /artifacts/kube/ \
 && cp -a /build/prompt/* /artifacts/prompt/ \
 && cp -a /build/prompt-root/* /artifacts/

# ========= STAGE 2: runtime minimal =========
FROM debian:bookworm-slim AS runtime

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates curl libcurl4 tzdata \
 && rm -rf /var/lib/apt/lists/*

# Dossier où le volume sera monté par docker-compose
# et vers lequel on synchronisera les artefacts la première fois
ENV DM_HOME=/opt/darkmoon

# Artefacts buildés (binaries + kube + prompt + txts)
COPY --from=builder /artifacts /opt/darkmoon-image

# Entrypoint qui copie au premier run vers $DM_HOME si vide
COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

# Le PATH inclura les binaires une fois copiés dans $DM_HOME
ENV PATH="$DM_HOME:$DM_HOME/kube:$PATH"

WORKDIR /opt/darkmoon
ENTRYPOINT ["/entrypoint.sh"]
# Par défaut, on ne lance rien d'intrusif ; on laisse l'opérateur piloter.
CMD ["bash", "-lc", "sleep infinity"]