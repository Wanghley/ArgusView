# ArgusView Backend

This directory contains the backend server code for ArgusView, responsible for processing images, generating descriptions, and creating audio responses.

## Overview

The backend is built with Node.js and Express. Its primary functions include:

- Receiving images from the ESP32-CAM device
- Processing images through OpenAI Vision APIs for description
- Generating audio descriptions of the scenes
- Handling optional audio context from the device

## Environment Setup

Create a `.env` file in the root of the backend directory with the following variables:

```
OPENAI_API_KEY=your_openai_api_key
PORT=3000 (optional, defaults to 3000)
```

## Installation and Running

```bash
# Install dependencies
npm install

# For development with hot-reload
npm run dev

# For production build
npm run build
npm start
```
