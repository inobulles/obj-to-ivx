
/// based off of http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-9-vbo-indexing/

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <map>

#include <glm/glm.hpp>
using namespace glm;

#ifndef VERSION_MAJOR
	#define VERSION_MAJOR 1
#endif

#ifndef VERSION_MINOR
	#define VERSION_MINOR 1
#endif

#define USE_NORMALS (VERSION_MAJOR * 100 + VERSION_MINOR) >= 101 // is version greater or equal to 1.1?

#ifndef IS_INDEX_SIZE_SHORT
	#define IS_INDEX_SIZE_SHORT 0
#endif

#ifndef AQUA_READABLE
	#define AQUA_READABLE 0
#endif

#if IS_INDEX_SIZE_SHORT /// THIS IS AUTOMATICALLY MANAGED, DO NOT TOUCH
	#define INDEX_TYPE uint16_t
#else
	#define INDEX_TYPE uint32_t
#endif

bool load_obj(const char* path, char* name, std::vector<glm::vec3>& out_vertices, std::vector<glm::vec2>& out_coords, std::vector<glm::vec3>& out_normals) {
	std::vector<uint32_t> vertex_indices;
	std::vector<uint32_t> coords_indices;
	std::vector<uint32_t> normal_indices;
	
	std::vector<glm::vec3> temp_vertices;
	std::vector<glm::vec2> temp_coords;
	std::vector<glm::vec3> temp_normals;
	
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
			temp_vertices.push_back(vertex);
			
		} else if (strcmp(line_header, "vt") == 0) {
			glm::vec3 coords;
			fscanf(file, "%f %f\n", &coords.x, &coords.y);
			coords.y = 1.0f - coords.y;
			temp_coords.push_back(coords);
			
		} else if (strcmp(line_header, "vn") == 0) {
			glm::vec3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);
			
		} else if (strcmp(line_header, "f") == 0) { // only works for triangles
			std::string vertex1;
			std::string vertex2;
			std::string vertex3;
			
			uint32_t vertex_index[3];
			uint32_t coords_index[3];
			uint32_t normal_index[3];
			
			if (fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertex_index[0], &coords_index[0], &normal_index[0], &vertex_index[1], &coords_index[1], &normal_index[1], &vertex_index[2], &coords_index[2], &normal_index[2]) != 9) {
				printf("ERROR OBJ file could not be read, try exporting with different options\n");
				return true;
			}
			
			vertex_indices.push_back(vertex_index[0]);
			vertex_indices.push_back(vertex_index[1]);
			vertex_indices.push_back(vertex_index[2]);
			
			coords_indices.push_back(coords_index[0]);
			coords_indices.push_back(coords_index[1]);
			coords_indices.push_back(coords_index[2]);
			
			normal_indices.push_back(normal_index[0]);
			normal_indices.push_back(normal_index[1]);
			normal_indices.push_back(normal_index[2]);
			
		} else {
			char temp[1024];
			fgets(temp, sizeof(temp), file);
		}
	}
	
	for (uint32_t i = 0; i < vertex_indices.size(); i++) {
		uint32_t vertex_index = vertex_indices[i];
		uint32_t coords_index = coords_indices[i];
		uint32_t normal_index = normal_indices[i];
		
		glm::vec3 vertex = temp_vertices[vertex_index - 1];
		glm::vec2 coords = temp_coords  [coords_index - 1];
		glm::vec3 normal = temp_normals [normal_index - 1];
		
		out_vertices.push_back(vertex);
		out_coords  .push_back(coords);
		out_normals .push_back(normal);
	}
	
	return false;
}

bool is_near(float x, float y) {
	return fabs(x - y) < 0.01f;
}

struct packed_vertex {
	glm::vec3 vertex;
	glm::vec2 coords;
	glm::vec3 normal;
	
	bool operator < (const packed_vertex that) const {
		return memcpy((void*) this, (void*) &that, sizeof(packed_vertex)) > 0;
	};
};

bool get_similar_vertex_index(packed_vertex& packed, std::map<packed_vertex, INDEX_TYPE>& vertex_to_out_index, INDEX_TYPE& result) {
	std::map<packed_vertex, INDEX_TYPE>::iterator iter = vertex_to_out_index.find(packed);
	
	if (iter == vertex_to_out_index.end()) {
		return false;
		
	} else {
		result = iter->second;
		return true;
	}
}

void index_vbo(std::vector<glm::vec3>& in_vertices, std::vector<glm::vec2>& in_coords, std::vector<glm::vec3>& in_normals, std::vector<INDEX_TYPE>& out_indices, std::vector<glm::vec3>& out_vertices, std::vector<glm::vec2>& out_coords, std::vector<glm::vec3>& out_normals) {
	std::map<packed_vertex, INDEX_TYPE> vertex_to_out_index;
	
	for (uint32_t i = 0; i < in_vertices.size(); i++) {
		packed_vertex packed = { .vertex = in_vertices[i], .coords = in_coords[i], .normal = in_normals[i] };
		INDEX_TYPE index;
		
		if (get_similar_vertex_index(packed, vertex_to_out_index, index)) {
			out_indices.push_back(index);
			
		} else {
			out_vertices.push_back(in_vertices[i]);
			out_coords  .push_back(in_coords  [i]);
			out_normals .push_back(in_normals [i]);
			
			INDEX_TYPE new_index = (INDEX_TYPE) out_vertices.size() - 1;
			out_indices.push_back(new_index);
			vertex_to_out_index[packed] = new_index;
		}
	}
}

typedef struct { // header
	uint64_t version_major;
	uint64_t version_minor;
	uint64_t is_index_size_short;
	uint8_t  name[1024];
	
	uint64_t vertex_count;
	uint64_t coords_count;
	uint64_t  index_count;
	
	uint64_t vertex_bytes; // size of a single element
	uint64_t coords_bytes;
	uint64_t  index_bytes;
	
	#if USE_NORMALS // is above or equal to version 1.1?
		uint64_t normal_count;
		uint64_t normal_bytes;
	#endif
} ivx_header_t;

int main(int argc, char* argv[]) {
	printf("Wavefront OBJ to Inobulles IVX file converter v%d.%d\n", VERSION_MAJOR, VERSION_MINOR);
	
	#if !AQUA_READABLE
		if (sizeof(float) != 4) {
			printf("ERROR IVX uses 32 bit floats when AQUA_READABLE is not set. Your platform does not seem to support 32 bit floats\n");
			return 1;
		}
	#endif
	
	if (argc < 2) {
		printf("ERROR You must specify which Wavefront file you want to convert\n");
		return 1;
	}
	
	for (int i = 1; i < argc; i++) {
		const char* argument = argv[i]; // parse arguments here
	}
	
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> coords;
	std::vector<glm::vec3> normals;
	
	ivx_header_t ivx;
	memset(&ivx, 0, sizeof(ivx));
	ivx.is_index_size_short = IS_INDEX_SIZE_SHORT;
	
	ivx.version_major = VERSION_MAJOR;
	ivx.version_minor = VERSION_MINOR;
	
	printf("Opening OBJ file (%s) ...\n", argv[1]);
	if (load_obj(argv[1], (char*) ivx.name, vertices, coords, normals)) {
		printf("ERROR An error occured while attempting to read OBJ file\n");
		return 1;
		
	} else {
		printf("Opened OBJ file, indexing ...\n");
	}
	
	std::vector<glm::vec3> indexed_vertices;
	std::vector<glm::vec2> indexed_coords;
	std::vector<glm::vec3> indexed_normals;
	
	std::vector<INDEX_TYPE> indices;
	index_vbo(vertices, coords, normals, indices, indexed_vertices, indexed_coords, indexed_normals);
	
	const char* ivx_path = argc > 2 ? argv[2] : "output.ivx";
	printf("OBJ file indexed, writing to IVX file (%s) ...\n", ivx_path);
	
	ivx.vertex_count = indexed_vertices.size();
	ivx.coords_count = indexed_coords  .size();
	ivx. index_count = indices         .size();
	
	#if USE_NORMALS
		ivx.normal_count = indexed_normals.size();
	#endif
	
	#if AQUA_READABLE
		#define convert_float(x) ((uint64_t) (x * 1000000))
		
		typedef struct { uint64_t x; uint64_t y; uint64_t z; } ivx_vertex_t;
		typedef struct { uint64_t x; uint64_t y;             } ivx_coords_t;
		typedef struct { uint64_t x; uint64_t y; uint64_t z; } ivx_normal_t;
	#else
		#define convert_float(x) (x)
		
		typedef struct { float x; float y; float z; } ivx_vertex_t;
		typedef struct { float x; float y;          } ivx_coords_t;
		typedef struct { float x; float y; float z; } ivx_normal_t;
	#endif
	
	ivx.vertex_bytes = sizeof(ivx_vertex_t);
	ivx.coords_bytes = sizeof(ivx_coords_t);
	ivx. index_bytes = ivx.is_index_size_short ? sizeof(uint16_t) : sizeof(uint32_t);
	
	#if USE_NORMALS
		ivx.normal_bytes = sizeof(ivx_normal_t);
	#endif
	
	FILE* ivx_file = fopen(ivx_path, "wb");
	if (ivx_file == NULL) {
		printf("ERROR Failed to open IVX file\n");
		return 1;
	}
	
	fwrite(&ivx, 1, sizeof(ivx), ivx_file);
	
	for (uint64_t i = 0; i < ivx.vertex_count; i++) {
		ivx_vertex_t data = { convert_float(indexed_vertices[i].x), convert_float(indexed_vertices[i].y), convert_float(indexed_vertices[i].z) };
		fwrite(&data, 1, sizeof(data), ivx_file);
		
	} for (uint64_t i = 0; i < ivx.coords_count; i++) {
		ivx_coords_t data = { convert_float(indexed_coords[i].x), convert_float(indexed_coords[i].y) };
		fwrite(&data, 1, sizeof(data), ivx_file);
		
	} for (uint64_t i = 0; i < ivx.index_count; i++) {
		if (ivx.is_index_size_short) {
			uint16_t data = indices[i];
			fwrite(&data, 1, sizeof(data), ivx_file);
			
		} else {
			uint32_t data = indices[i];
			fwrite(&data, 1, sizeof(data), ivx_file);
		}
	}
	
	#if USE_NORMALS
		for (uint64_t i = 0; i < ivx.normal_count; i++) {
			ivx_normal_t data = { convert_float(indexed_normals[i].x), convert_float(indexed_normals[i].y), convert_float(indexed_normals[i].z) };
			fwrite(&data, 1, sizeof(data), ivx_file);
		}
	#endif
	
	printf("Finished converting successfully\n");
	
	fclose(ivx_file);
	return 0;
}

