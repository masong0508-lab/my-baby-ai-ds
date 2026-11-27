# Performance baseline — 2026-11-27

## Verified baseline

Source: PR #90, head `6fc2dd5a69acdf72caecf888f471816d3e203011`, merged as `75a7d70bbf029fbe101bc60a04100d4d192c0880`. The standard DS and BlocksDS workflows passed all 11 jobs. Both the native stylus job and classic/plain ROM boot job printed the same samples.

| Scene | Current | Average | Low | High |
| --- | ---: | ---: | ---: | ---: |
| Nursery | 29.9 | 29.5 | 28.9 | 30.1 |
| Room | 29.9 | 28.9 | 27.9 | 29.9 |
| Toybox | 29.9 | 29.2 | 28.4 | 29.9 |
| Arcade menu | 29.9 | 29.4 | 28.5 | 29.9 |

These are completed frame submissions per emulated DS second, sampled with the in-ROM hardware-clock tracker; average/min/max include previous completed one-second windows in that scene. They are not host wall-clock speed, independent LCD scanout FPS, or original-hardware benchmarks. The previous native test did not independently verify the visible view titles at measurement time; the integrated test adds that check.

Evidence: [native DeSmuME job](https://github.com/masong0508-lab/my-baby-ai-ds/actions/runs/46009629462/job/140543176714).

## Next integrated measurement

PR #91 combines the nursery features, geometry/raster updates and FPS baseline. It adds per-scene color-command deduplication and reuses ring coordinates and packed RGB15 shading for the 8-sided ellipsoids and 12-sided soft meshes. The latter reduces shade calculations from 27 to 5 per ellipse and from 130 to 15 per soft-head mesh; topology and shading are intended to be identical. These are source-level work reductions, not measured FPS gains.

Fresh native reports must verify the four original views plus Diaper Dash, Talk, WARDEN, microphone lab and album. Compare exact-head results and check native screenshots and the existing geometry budget. Targets: stable approximately 30 submitted FPS first, then pursue higher throughput without reducing animation quality or changing gameplay speed. Do not treat a queued build as evidence of improvement.
