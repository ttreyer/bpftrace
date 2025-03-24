#pragma once

#include <cstdint>
#include <span>
#include <unordered_map>

enum loc_type {
	LOC_END_OF_EXPR,
	LOC_SIGNED_CONST_1,
	LOC_SIGNED_CONST_2,
	LOC_SIGNED_CONST_4,
	LOC_SIGNED_CONST_8,
	LOC_UNSIGNED_CONST_1,
	LOC_UNSIGNED_CONST_2,
	LOC_UNSIGNED_CONST_4,
	LOC_UNSIGNED_CONST_8,
	LOC_REGISTER,
} __attribute__((packed));

struct loc {
	enum loc_type type;
	uint8_t size;
};

struct inline_parameter {
	struct loc *location[16];
};

typedef unsigned int type_id_t;
struct inline_instance {
	uint64_t insn_offset;
	type_id_t type_id;
	uint16_t param_count;
	struct inline_parameter parameters[];
};

struct inline_info {
  struct inline_encoder__header *hdr;
	std::unordered_multimap<type_id_t, struct inline_instance *> instances;
  struct loc *locations;
};

struct inline_info *inline_info__parse(std::span<std::byte> data);