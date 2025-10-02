# Stage 1: Builder
FROM ubuntu:22.04 AS builder

# Set environment variables
ARG DEBIAN_FRONTEND=noninteractive
ENV QT_VERSION=6.6.0
ENV PRO_FILE=src/src.pro

# Install build dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    software-properties-common \
    gnupg \
    ca-certificates \
    && apt-key adv --fetch-key https://repo.download.nvidia.com/jetson/jetson-ota-public.asc \
    && add-apt-repository 'deb https://repo.download.nvidia.com/jetson/x86_64/jammy r36.4 main' \
    && apt-get update

COPY .github/packages.list .
RUN apt-get update && \
    apt-get install -y --no-install-recommends dos2unix && \
    dos2unix packages.list && \
    PACKAGES=$(grep -vE '^[[:space:]]*#|^[[:space:]]*$' packages.list | tr '\n' ' ') && \
    if [ -n "$PACKAGES" ]; then apt-get install -y --no-install-recommends $PACKAGES; fi

# Install Python and Qt
RUN apt-get install -y --no-install-recommends python3-pip && \
    pip3 install aqtinstall==3.1.8 && \
    aqt install-qt --outputdir /opt/Qt/ linux desktop ${QT_VERSION} -m qtserialbus qtserialport qtmultimedia

# Set Qt environment variables
ENV PATH="/opt/Qt/${QT_VERSION}/gcc_64/bin:${PATH}"
ENV LD_LIBRARY_PATH="/opt/Qt/${QT_VERSION}/gcc_64/lib:${LD_LIBRARY_PATH}"
ENV QT_PLUGIN_PATH="/opt/Qt/${QT_VERSION}/gcc_64/plugins"
ENV QML2_IMPORT_PATH="/opt/Qt/${QT_VERSION}/gcc_64/qml"

# Copy source code and build
COPY . /app
WORKDIR /app
RUN mkdir build && \
    cd build && \
    qmake ../${PRO_FILE} && \
    make -j$(nproc)

# Stage 2: Runtime
FROM ubuntu:22.04

# Set environment variables
ARG DEBIAN_FRONTEND=noninteractive
ENV QT_VERSION=6.6.0

# Install runtime dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    libglib2.0-0 \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-libav \
    libx11-6 \
    libxext6 \
    libxcb1 \
    libxcb-glx0 \
    libxcb-xinerama0 \
    libxcb-icccm4 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-randr0 \
    libxcb-render-util0 \
    libxcb-shape0 \
    libxcb-xkb1 \
    libxkbcommon-x11-0 \
    libfontconfig1 \
    libdbus-1-3 \
    libfreetype6 \
    libicu70 \
    libinput10 \
    libjpeg-turbo8 \
    libpng16-16 \
    libzstd1 \
    libmd4c0 \
    libharfbuzz-icu0 \
    libharfbuzz0b \
    libproxy1v5 \
    libbrotli1 && \
    rm -rf /var/lib/apt/lists/*

# Copy Qt libraries from builder
COPY --from=builder /opt/Qt/${QT_VERSION}/gcc_64/lib /usr/local/lib/
COPY --from=builder /opt/Qt/${QT_VERSION}/gcc_64/plugins /usr/local/plugins/

# Copy application binary from builder
COPY --from=builder /app/build/src /app/

# Set environment variables for runtime
ENV LD_LIBRARY_PATH=/usr/local/lib
ENV QT_QPA_PLATFORM_PLUGIN_PATH=/usr/local/plugins
ENV QT_QPA_PLATFORM=xcb

# Set the entrypoint
WORKDIR /app
ENTRYPOINT ["./src"]