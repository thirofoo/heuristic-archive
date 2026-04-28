#!/bin/bash

cd visualizer
echo "Installing dependencies..."
yarn
cd wasm
echo "Building wasm..."
wasm-pack build --target web --out-dir ../public/wasm
cd ..
echo "Starting development server..."
yarn dev
