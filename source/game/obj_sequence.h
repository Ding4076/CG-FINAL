#pragma once

#include "model.h"

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

// OBJ-sequence animation (course base requirement 7): plays back a series of
// frame_XXX.obj meshes in order, swapping the active mesh per frame. This is
// genuine mesh animation -- the vertex positions differ per frame, so the
// geometry itself deforms (a simple translate/rotate/scale is NOT considered
// animation playback by the spec).
//
// Self-contained: if the frame directory is empty/missing, the frames are
// generated on first run (a pulsing/rippling sphere) and written to disk, then
// read straight back -- so the "read consecutive .obj files" requirement is met
// without shipping any binary asset.
class ObjSequence {
public:
    // Load `count` frames named "<dir>/frame_000.obj" ... "<dir>/frame_<count-1>.obj".
    // If frame_000.obj is absent, every frame is generated first (self-contained
    // demo). `fps` is the playback frame rate. Returns false only on total failure.
    bool loadOrGenerate(const std::string& dir, int count, int fps = 12);

    // Advance playback by dt seconds (scaled by speed). Swaps the active mesh
    // once enough time for the next frame has elapsed.
    void update(float dt);

    void rewind();
    void setLooping(bool loop) { _looping = loop; }
    void setSpeed(float speed) { _speed = speed > 0.0f ? speed : 1.0f; }

    bool finished() const { return _done; }
    int frameCount() const { return static_cast<int>(_frames.size()); }
    int currentFrame() const { return _index; }

    // The mesh to draw this frame. The caller binds the shader and sets uModel
    // + material uniforms, then calls currentModel()->draw().
    Model* currentModel() const;

private:
    std::vector<std::unique_ptr<Model>> _frames;
    int _index = 0;
    float _accum = 0.0f;          // seconds accumulated toward the next frame swap
    float _frameTime = 1.0f / 12.0f;
    bool _looping = false;
    bool _done = false;
    float _speed = 1.0f;
};
