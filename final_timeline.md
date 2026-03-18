# Final Project Timeline
**Project:** Shadow Maps + Normal Mapping
**Start:** 2026-03-17 | **Presentations:** ~2026-04-21 (5 weeks)

---

## Week 1 (Mar 17–23) — Shadow Map Infrastructure

**Goal:** Get a depth pass rendering into an FBO.

- [ ] Create `ShadowMap` class (`scene/shadow_map.cpp/.hpp`)
  - Owns an FBO + depth texture (e.g. 1024×1024 or 2048×2048)
  - `bind()` / `unbind()` helpers for the two-pass loop
- [ ] Write `depth_pass.vert` / `depth_pass.frag` shaders (SampleProject)
  - Vertex shader: just `gl_Position = light_pvm * vec4(vtx_position, 1.0)`
  - Fragment shader: empty (depth writes automatically)
- [ ] Add `LightPVMatrix` uniform plumbing — compute from the key light's position
- [ ] Wire a two-pass render loop in `SampleProject/main.cpp`
  - Pass 1: bind shadow FBO, render scene with depth shader, unbind
  - Pass 2: normal scene render (shadow map not yet used — just verify no crash)

**Milestone:** Scene renders normally; FBO created without GL errors.

---

## Week 2 (Mar 24–30) — Shadow Lookup in Fragment Shader

**Goal:** Objects cast and receive shadows.

- [ ] Extend `pixel_lighting_tex.vert` to output `shadow_coord` (vertex in light clip space)
  - Add `uniform mat4 light_pvm_matrix`
  - Output `smooth out vec4 shadow_coord`
- [ ] Extend `pixel_lighting_tex.frag` to sample shadow map
  - Add `uniform sampler2DShadow shadow_map` + `uniform int use_shadow`
  - Use `textureProj(shadow_map, shadow_coord)` to get shadow factor
  - Multiply diffuse + specular contributions by shadow factor
- [ ] Extend `LightingShaderNode` to bind the depth texture and upload the light matrix
- [ ] Tune depth bias to eliminate self-shadowing acne
  - Try `shadow_coord.z -= bias` in the vert shader, or use `glPolygonOffset` during depth pass

**Milestone:** Hard shadows visible; some acne/peter-panning — that's expected.

---

## Week 3 (Apr 1–7) — PCF + Shadow Polish

**Goal:** Shadows look good; no acne.

- [ ] Implement PCF (percentage-closer filtering) in the fragment shader
  - Sample a 3×3 kernel around `shadow_coord.xy` using `textureOffset`
  - Average the 9 binary comparisons → soft shadow penumbra
- [ ] Tune bias + PCF kernel size until shadows look clean
- [ ] Test with multiple objects (table, cone, Bezier patch) casting shadows onto the floor
- [ ] Verify directional vs. point light shadow framing (orthographic vs. perspective light projection)
- [ ] Start scene design — sketch what the final scene will look like

**Milestone:** Soft, clean shadows on several objects; scene layout decided.

---

## Week 4 (Apr 8–14) — Normal Mapping

**Goal:** At least one surface has normal-mapped detail.

- [ ] Add tangent attribute to `TriSurface` / `TexturedTriSurface`
  - Compute per-vertex tangents from UV deltas during tessellation
  - Add `vtx_tangent` (location 3) to vertex buffer + VAO
- [ ] Extend vertex shader: pass `tangent`, `bitangent`, `normal` to fragment shader
- [ ] Extend fragment shader
  - Add `uniform sampler2D normal_map` + `uniform int use_normal_map`
  - Build TBN matrix from interpolated T/B/N
  - Sample normal map, remap `[0,1]→[-1,1]`, transform to world space
  - Use this perturbed normal for all lighting calculations
- [ ] Extend `LightingShaderNode` / `TextureNode` to bind a second texture unit
- [ ] Apply normal map to 1–2 surfaces (floor tiles, wall, Bezier patch dome)

**Milestone:** Visibly bumpy surface; lighting responds to surface detail.

---

## Week 5 (Apr 15–21) — Scene, Documentation, Presentation

**Goal:** Polished deliverable ready for class.

- [ ] Finalize creative scene
  - Multiple objects casting/receiving shadows
  - At least one normal-mapped material
  - Good lighting placement to show off both techniques
  - Camera presets / keyboard shortcuts for the demo
- [ ] Write README updates (new keyboard commands, build instructions)
- [ ] Create 5–7 slide presentation
  - Slide 1: title + motivation
  - Slide 2: shadow map algorithm overview (diagram)
  - Slide 3: PCF explanation + before/after screenshots
  - Slide 4: normal mapping algorithm overview (TBN matrix diagram)
  - Slide 5: before/after normal map screenshots
  - Slide 6: final scene screenshots
  - Slide 7 (optional): challenges / what you'd do next
- [ ] Take screenshots for slides + README
- [ ] Final build test on a clean checkout; verify no stray asset paths

**Milestone:** Presentation ready; code submitted before class day.

---

## Risk / Contingency

| Risk | Mitigation |
|------|------------|
| Shadow acne is hard to fully eliminate | Accept slight acne, document bias tradeoff in slides |
| Tangent computation is tricky for some meshes | Limit normal mapping to flat/quad surfaces (floor, Bezier patch) |
| Running out of time | Drop PCF (use hard shadows) and/or drop normal mapping — shadows alone are enough |
| Scene looks boring | Reuse existing scene objects (teapot, vase, globe) — just add a strong directional light |

---

## Key Dates

| Date | Event |
|------|-------|
| 2026-03-17 | Project starts; submit idea to instructor |
| 2026-03-23 | FBO depth pass working |
| 2026-03-30 | Shadow lookup in shader working |
| 2026-04-07 | PCF + shadow polish done; scene layout decided |
| 2026-04-14 | Normal mapping working on at least one surface |
| 2026-04-21 | Code submitted; presentation ready |
