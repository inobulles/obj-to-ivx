
/// based off of http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-9-vbo-indexing/

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <string.h>
#include <map>

#include <glm/glm.hpp>
using namespace glm;

#ifndef VERSION_MAJOR
	#define VERSION_MAJOR 2
#endif

#ifndef VERSION_MINOR
	#define VERSION_MINOR 1
#endif

#define USE_NORMALS (VERSION_MAJOR * 100 + VERSION_MINOR) >= 101 // is version greater or equal to 1.1?
#define MAX_ATTRIBUTE_COUNT 8

bool load_obj(const char* path, char* name, std::vector<glm::vec3>& out_vertices, std::vector<glm::vec2>& out_coords, std::vector<uint32_t>& out_indices) {
	FILE* file = fopen(path, "r");
	if (file == NULL) {
		printf("ERROR Failed to open OBJ file\n");
		return true;
	}
	
	while (1) {
		char line_header[128];
		if (fscanf(file, "%s", line_header) == EOF) {
			break;
			
		} else if (strcmp(line_header, "o") == 0) {
			fscanf(file, "%s\n", name);
			
		} else if (strcmp(line_header, "v") == 0) {
			glm::vec3 vertex;
			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			printf("v %f %f %f\n", vertex.x, vertex.y, vertex.z);
			out_vertices.push_back(vertex);
			
		} else if (strcmp(line_header, "vt") == 0) {
			glm::vec2 coords;
			fscanf(file, "%f %f\n", &coords.x, &coords.y);
			out_coords.push_back(coords);
			
		} else if (strcmp(line_header, "f") == 0) { // only works for triangles
			std::string vertex1;
			std::string vertex2;
			std::string vertex3;
			
			uint32_t vertex_index[3];
			uint32_t coords_index[3];
			
			if (fscanf(file, "%d/%d %d/%d %d/%d\n", &vertex_index[0], &coords_index[0], &vertex_index[1], &coords_index[1], &vertex_index[2], &coords_index[2]) != 6) {
				printf("ERROR OBJ file could not be read, try exporting with different options\n");
				return true;
			}
			
			out_indices.push_back(vertex_index[0]);
			out_indices.push_back(vertex_index[1]);
			out_indices.push_back(vertex_index[2]);
			
		} else {
			char temp[1024];
			fgets(temp, sizeof(temp), file);
		}
	}

	return false;
}

bool is_near(float x, float y) {
	return fabs(x - y) < 1.f;
}

struct packed_vertex_s {
	glm::vec3 vertex;
	glm::vec2 coords;
	
	bool operator < (const packed_vertex_s that) const {
		return memcpy((void*) this, (void*) &that, sizeof(packed_vertex_s)) > 0;
	};
};

bool get_similar_vertex_index(packed_vertex_s& packed, std::map<packed_vertex_s, uint32_t>& vertex_to_out_index, uint32_t& result) {
	std::map<packed_vertex_s, uint32_t>::iterator iter = vertex_to_out_index.find(packed);
	
	if (iter == vertex_to_out_index.end()) {
		return false;
		
	} else {
		result = iter->second;
		return true;
	}
}

void index_vbo(std::vector<glm::vec3>& in_vertices, std::vector<glm::vec2>& in_coords, std::vector<uint32_t>& out_indices, std::vector<glm::vec3>& out_vertices, std::vector<glm::vec2>& out_coords) {
	std::map<packed_vertex_s, uint32_t> vertex_to_out_index;
	
	for (uint32_t i = 0; i < in_vertices.size(); i++) {
		packed_vertex_s packed = { .vertex = in_vertices[i], .coords = in_coords[i] };
		uint32_t index;
		
		if (get_similar_vertex_index(packed, vertex_to_out_index, index)) {
			out_indices.push_back(index);
			
		} else {
			out_vertices.push_back(in_vertices[i]);
			out_coords  .push_back(in_coords  [i]);
			
			uint32_t new_index = (uint32_t) out_vertices.size() - 1;
			out_indices.push_back(new_index);
			vertex_to_out_index[packed] = new_index;
		}
	}
}

typedef struct { // vertex attribute header
	uint64_t components;
	uint64_t offset;
} ivx_attribute_header_t;

typedef struct { // header
	uint64_t version_major;
	uint64_t version_minor;
	uint8_t  name[1024];
	
	uint64_t index_count;
	uint64_t index_offset;
	
	uint64_t vertex_count;
	ivx_attribute_header_t attributes[MAX_ATTRIBUTE_COUNT];
} ivx_header_t;

typedef struct { float x; float y; float z; } ivx_vec3_t;
typedef struct { float x; float y;          } ivx_vec2_t;

int main(int argc, char* argv[]) {
	printf("Wavefront OBJ to Inobulles IVX file converter v%d.%d\n", VERSION_MAJOR, VERSION_MINOR);
	
	if (sizeof(float) != 4) {
		printf("ERROR IVX uses 32 bit floats. Your platform does not seem to support 32 bit floats\n");
		return 1;
	}
	
	if (argc < 2) {
		printf("ERROR You must specify which Wavefront file you want to convert\n");
		return 1;
	}
	
	for (int i = 1; i < argc; i++) {
		const char* argument = argv[i]; // parse arguments here
	}
	
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> coords;
	std::vector<uint32_t> indices;
	
	ivx_header_t ivx_header;
	memset(&ivx_header, 0, sizeof(ivx_header));
	
	ivx_header.version_major = VERSION_MAJOR;
	ivx_header.version_minor = VERSION_MINOR;
	
	printf("Opening OBJ file (%s) ...\n", argv[1]);
	if (load_obj(argv[1], (char*) ivx_header.name, vertices, coords, indices)) {
		printf("ERROR An error occured while attempting to read OBJ file\n");
		return 1;
		
	} else {
		printf("Opened OBJ file, indexing ...\n");
	}
	
	ivx_header.vertex_count = vertices.size();
	ivx_header.index_count = indices.size();

	printf("%ld indices\n", ivx_header.index_count);
	
	const char* ivx_path = argc > 2 ? argv[2] : "output.ivx";
	printf("OBJ file indexed, writing to IVX file (%s) ...\n", ivx_path);
	
	FILE* ivx_file = fopen(ivx_path, "wb");
	if (ivx_file == NULL) {
		printf("ERROR Failed to open IVX file\n");
		return 1;
	}
	
	fwrite(&ivx_header, 1, sizeof(ivx_header), ivx_file); // leave space for writing the header later
	
	// write indices
	
	ivx_header.index_offset = ftell(ivx_file);
	for (uint32_t i = 0; i < ivx_header.index_count; i++) {
		uint32_t data = indices[i];
		fwrite(&data, 1, sizeof(data), ivx_file);
	}
	
	// write vertex attribute
	
	ivx_header.attributes[0].components = 3;
	ivx_header.attributes[0].offset = ftell(ivx_file);
	
	for (uint64_t i = 0; i < vertices.size(); i++) {
		ivx_vec3_t data = { vertices[i].x, vertices[i].y, vertices[i].z };
		fwrite(&data, 1, sizeof(data), ivx_file);
	}
	
	// write texture coord attribute
	
	ivx_header.attributes[1].components = 2;
	ivx_header.attributes[1].offset = ftell(ivx_file);
	
	for (uint64_t i = 0; i < coords.size(); i++) {
		ivx_vec2_t data = { coords[i].x, coords[i].y };
		fwrite(&data, 1, sizeof(data), ivx_file);
		
	}
	
	// go back and write the header
	
	rewind(ivx_file);
	fwrite(&ivx_header, 1, sizeof(ivx_header), ivx_file);
	
	printf("Finished converting successfully\n");
	
	fclose(ivx_file);
	return 0;
}

