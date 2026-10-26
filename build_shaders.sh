find assets -type f -name "*.hlsl" -exec sh -c 'for f; do ./bin/shadercross "$f" -o "${f%.oldext}.spv"; done' _ {} +
find assets -type f -name "*.hlsl" -exec sh -c 'for f; do ./bin/shadercross "$f" -o "${f%.oldext}.dxil"; done' _ {} +
find assets -type f -name "*.hlsl" -exec sh -c 'for f; do ./bin/shadercross "$f" -o "${f%.oldext}.msl"; done' _ {} +
find assets -type f -name "*.hlsl" -exec sh -c 'for f; do ./bin/shadercross "$f" -d JSON -o "${f%.oldext}.meta.json"; done' _ {} +
