# FEED animation: safe hand cap geometry

Native DeSmuME evidence on the existing PR #63 ROM exposed a brown horizontal
streak during FEED. In `game_nursery_after_touch_feed.png`, the abnormal warm
run spans 77 pixels beginning at `(38,113)`. The equivalent idle and CHAT
captures have a maximum 28-pixel run in that region, belonging to the teddy.

The articulated hands used the general ellipsoid's quad-strip pole bands. Each
pole quad repeats one position with two different vertex colors; a rotated cap
can consequently become a nearly horizontal polygon with conflicting colors at
coincident vertices. The left upper hand cap projects near the offending row
during FEED, and the streak's light end matches the shaded hand material.

Both hands now use genuine triangles at their two caps, with one stable pole
color, while retaining the original eight-sided middle band. The silhouette,
shoulder/hand attachment and 48 combined hand polygons stay the same. The two
hands add 24 submitted vertices per nursery frame. No other ellipsoid, including
the egg, changes. No texture, heap allocation, framebuffer or save field is added.

Host geometry hooks inspect the emitted triangle vertices and reject zero-area
triangles. A full care-age sweep requires all 32 hand-cap triangles and enforces
the existing DS budgets. The complete renderer suite passes with a maximum of
4,702 submitted vertices and 1,808 polygons.

Native acceptance must check the fresh ROM: in the FEED screenshot's box
`x=16..114, y=100..123`, reject a horizontal run of 40 or more pixels satisfying
`red > green + 50`, `green < 190`, and `blue < 170`. Review the actual screenshot
for attached hands and retained shading. The old screenshot demonstrates the
defect; passing host topology checks alone does not prove its visual removal.
