
FROM node:21.2.0-slim

RUN apt-get update && apt-get install -y \
    chromium \
    libgconf-2-4 \
    libatk1.0-0 \
    libatk-bridge2.0-0 \
    libgdk-pixbuf2.0-0 \
    libgtk-3-0 \
    libnss3 \
    libx11-xcb1 \
    libxss1 \
    fonts-liberation \
    libappindicator3-1 \
    libasound2 \
    libdrm2 \
    libxkbcommon0 \
    xdg-utils \
    wget


ENV PUPPETEER_EXECUTABLE_PATH=/usr/bin/chromium
ENV PUPPETEER_SKIP_CHROMIUM_DOWNLOAD=true


WORKDIR /app


COPY package.json package-lock.json* ./
RUN npm ci --omit=dev
RUN npx puppeteer browsers install chrome

COPY . .


EXPOSE 3000


CMD ["node", "index.js"]
