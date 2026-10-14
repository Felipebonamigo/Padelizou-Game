# Padelizou Game

Padel 2×2 arcade-realista no Unreal Engine 5. Design: [`docs/DESIGN.md`](docs/DESIGN.md). Estado: [`docs/estado-atual.md`](docs/estado-atual.md).

A física da bola (`Source/PadelSim`, C++ puro) é testada fora do Unreal contra o gabarito do `Padel.Core`:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build && ctest --test-dir build --output-on-failure
```
