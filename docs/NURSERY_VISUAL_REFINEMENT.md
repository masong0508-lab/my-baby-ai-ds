# Nursery facial surfaces, materials and contact

This pass refines the actual Nintendo DS renderer in `source/world.c`.
It keeps the existing warm, artificial baby-care nursery, camera presets,
connected doll rig, Toybox physics and optional egg film. It adds no image
assets, textures, framebuffer, heap allocation or new animation state.

## Findings from the previous native build

The source baseline is `ef3032046ee7685812ab196634ea408cec66cd2a`.
Its real DeSmuME captures showed conspicuous stacked white eye surfaces,
disk-like cheek patches, pale cloth blending into the bib and weak contact
between the booties and play mat. Window/lamp controls worked, but broad
room surfaces had little distinction between cool ambient light and the
warm dresser end. The sleeping face and connected care gestures already
read clearly and remain part of the acceptance review.

## Renderer changes

- **Eyes:** four extremely thin full ellipsoids per eye become shallow
  rounded surfaces. Eight positive-area triangles per layer give the DS
  GPU a soft center-to-edge color gradient without the hidden back half
  or repeated poles of the former meshes. A subdued rim, warm sclera,
  smaller teal iris and dark pupil preserve the doll's recognizable face.
- **Blinks:** the opening and iris narrow together. Low blinks omit the
  tiny catchlight, and full closure retains the existing joined crease
  marks. No rectangular skin shutter covers the cheeks. All facial parts
  still share the existing neck/attention transform.
- **Speech aperture:** native review also identified a pre-existing long
  dark line across the jaw at the smallest CHAT syllable. The mouth's thin
  ellipsoid now uses the same positive-area surface treatment at its old
  visible front depth. The tongue follows the opening instead of remaining
  below a nearly closed syllable. Toybox impact gasps share this correction.
- **Materials:** cheeks use the same shaded surface treatment. The bib
  becomes a soft cream cloth patch; the romper has more distinct teal
  midtones, and booties have broad, subdued toe trim instead of thin
  decorative marks. The boot portrait uses the same eye treatment.
- **Room light:** the existing broad back-wall and floor gradients gain
  clearer warm/cool separation. One broad left-wall surface connects the
  lighting around the room. These are geometry colors, not screen bloom
  or a translucent image effect.
- **Contact:** clipped-corner floor footprints anchor the dresser, crib
  and teddy. Two nested tones beneath the doll span the actual boot
  footprint. They move and contract with the existing Toybox simulation.

No care, conversation, save, camera, movement, RNG or sound behavior changes.
The optional creature geometry and its material colors are untouched.

## Submission budgets and host verification

The focused strict C11 world test passes. Before adding the WARDEN landmark,
the sampled nursery maximum is **4,394 submitted vertices / 1,629 polygons**,
down from **4,702 / 1,808**: 308 fewer vertices and 179 fewer polygons despite
the contact surfaces. The mouth correction alone saves another 30 vertices
and 16 polygons compared with the initial eye/material refinement.
With the physical WARDEN wall terminal and connected hostile poses integrated,
the same sampled maximum is **4,402 vertices / 1,631 polygons**.
This is a command-submission count, not measured FPS or total RAM usage.

The nursery regression gate is tightened to **4,500 vertices / 1,700
polygons**. The broader existing **5,000 / 1,850** gate remains for all
optional film/toy states, with the title still limited to **1,024 / 350**.
Tests check every emitted triangle for positive area, including partial
blinks. Hand-cap topology is counted from its existing pole coordinates
so adding facial triangles cannot disguise a missing hand cap. Existing
motion, state isolation, room camera and egg continuity tests still pass.

## Required native comparison

Review the candidate production ROM at 256×192 per screen before merging:

1. CARE and TALK: eye rims/irises must remain inset and readable, with no
   white plaques, face clipping, detached catchlights or added teeth.
   Low CHAT syllables must also stay within the intended mouth footprint;
   inspect the LIKE acknowledgement against its preserved before capture.
2. NAP, low energy and rapid care changes: check complete/partial blinks,
   connected face/arms and the return to idle.
3. ROOM window, lamp, teddy and crib views: check coherent broad lighting,
   contact under each object and no new vertical/horizontal streaks.
4. Toybox motion and egg entrance: the contact must follow the doll and
   the optional film must retain its existing coherent creature features.
5. Title: the portrait should match the nursery's eye design while boot
   rendering remains within its separate smaller budget.

The native integration suite also retains its pixel guards for long pale
streaks, vertical seams, cyan torso visibility and coherent state changes.
Host geometry tests do not substitute for these rendered comparisons or
for physical Nintendo DS performance and flashcart acceptance.

## Complete DS title portrait (2026-11-22)

A real DeSmuME 256x192 title screenshot after the loader patch showed a doll head emerging from the display plinth, with no visible arms or shoes and distracting pink background pillars. The renderer now adds joined sleeves, mittens, soft teal romper, cream bib and small feet; the camera is reframed and the plinth lowered. It removes the two tall pink poles. The polygons are original native DS geometry, without textures, allocations or new save state. Strict host title geometry remains <=1024 vertices and <=350 polygons, with a minimum detail submission to prevent recurrence. Real emulator screenshots and both DS toolchains are required before accepting the visual pass.
