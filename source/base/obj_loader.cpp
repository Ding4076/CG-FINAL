#include "obj_loader.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {

std::vector<std::string> splitWhitespace(const std::string& s) {
    std::vector<std::string> out;
    std::istringstream iss(s);
    std::string tok;
    while (iss >> tok) {
        out.push_back(tok);
    }
    return out;
}

// Parse a face-corner token of any of these forms:
//   "1"        -> p=1, t=-1, n=-1
//   "1/2"      -> p=1, t=2, n=-1
//   "1//3"     -> p=1, t=-1, n=3
//   "1/2/3"    -> p=1, t=2, n=3
// Indices are 1-based (as in .obj). Returns -1 for absent attributes.
RawObj::Corner parseCorner(const std::string& tok) {
    RawObj::Corner c;
    const size_t first = tok.find('/');
    if (first == std::string::npos) {
        c.p = std::stoi(tok);
        return c;
    }
    if (first > 0) {
        c.p = std::stoi(tok.substr(0, first));
    }
    const size_t second = tok.find('/', first + 1);
    if (second == std::string::npos) {
        // exactly one slash: "p/t"
        std::string t = tok.substr(first + 1);
        if (!t.empty()) {
            c.t = std::stoi(t);
        }
        return c;
    }
    // two slashes: "p/t/n" or "p//n"
    std::string mid = tok.substr(first + 1, second - first - 1);
    if (!mid.empty()) {
        c.t = std::stoi(mid);
    }
    std::string tail = tok.substr(second + 1);
    if (!tail.empty()) {
        c.n = std::stoi(tail);
    }
    return c;
}

} // namespace

RawObj parseObjString(const std::string& text) {
    RawObj obj;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        std::vector<std::string> parts = splitWhitespace(line);
        if (parts.empty()) {
            continue;
        }
        const std::string& tag = parts[0];
        if (tag == "v" && parts.size() >= 4) {
            obj.positions.emplace_back(
                std::stof(parts[1]), std::stof(parts[2]), std::stof(parts[3]));
        } else if (tag == "vt" && parts.size() >= 3) {
            obj.texcoords.emplace_back(std::stof(parts[1]), std::stof(parts[2]));
        } else if (tag == "vn" && parts.size() >= 4) {
            obj.normals.emplace_back(
                std::stof(parts[1]), std::stof(parts[2]), std::stof(parts[3]));
        } else if (tag == "f" && parts.size() >= 4) {
            std::vector<RawObj::Corner> face;
            face.reserve(parts.size() - 1);
            for (size_t i = 1; i < parts.size(); ++i) {
                face.push_back(parseCorner(parts[i]));
            }
            obj.faces.push_back(std::move(face));
        }
        // other tags (o, g, s, mtllib, usemtl, comments) are ignored
    }
    return obj;
}

RawObj loadObj(const std::string& filepath) {
    std::ifstream f(filepath);
    if (!f) {
        throw std::runtime_error("Cannot open obj: " + filepath);
    }
    std::stringstream ss;
    ss << f.rdbuf();
    return parseObjString(ss.str());
}

std::string writeObj(const std::vector<glm::vec3>& positions,
                     const std::vector<glm::vec3>& normals,
                     const std::vector<glm::vec2>& texcoords,
                     const std::vector<uint32_t>& indices) {
    std::ostringstream out;
    out << "# exported by AimTrainer\n";
    for (const auto& p : positions) {
        out << "v " << p.x << ' ' << p.y << ' ' << p.z << '\n';
    }
    for (const auto& t : texcoords) {
        out << "vt " << t.x << ' ' << t.y << '\n';
    }
    for (const auto& n : normals) {
        out << "vn " << n.x << ' ' << n.y << ' ' << n.z << '\n';
    }
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        // Emit "p/t/n" with the same 1-based index for all three attributes
        // (the Model's deduplicated layout bundles pos+normal+uv per vertex).
        const uint32_t a = indices[i] + 1;
        const uint32_t b = indices[i + 1] + 1;
        const uint32_t c = indices[i + 2] + 1;
        out << "f " << a << '/' << a << '/' << a << ' ' << b << '/' << b << '/' << b << ' '
            << c << '/' << c << '/' << c << '\n';
    }
    return out.str();
}

void saveObj(const std::string& filepath,
             const std::vector<glm::vec3>& positions,
             const std::vector<glm::vec3>& normals,
             const std::vector<glm::vec2>& texcoords,
             const std::vector<uint32_t>& indices) {
    std::ofstream f(filepath);
    if (!f) {
        throw std::runtime_error("Cannot write obj: " + filepath);
    }
    f << writeObj(positions, normals, texcoords, indices);
}
