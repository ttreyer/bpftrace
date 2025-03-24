#include "inline.h"
#include <cstdlib>
#include <cstring>

struct inline_encoder__header {
	uint16_t magic;
	uint8_t version;
	uint8_t flags;
	uint32_t header_size;
	uint32_t inline_info_offset;
	uint32_t inline_info_size;
	uint32_t location_offset;
	uint32_t location_size;
};

static void inline_info__parse_instances(struct inline_info *info) {
	auto *cursor = reinterpret_cast<std::byte*>(info->hdr) + info->hdr->inline_info_offset;
	const auto *end = cursor + info->hdr->inline_info_size;
	while (cursor < end) {
		uint16_t param_count = 0;
		std::memcpy(&param_count, cursor + offsetof(struct inline_instance, param_count), sizeof(param_count));
		auto instance_size = sizeof(struct inline_instance) + param_count * sizeof(struct inline_parameter);
		auto *instance = reinterpret_cast<struct inline_instance*>(malloc(instance_size));
		std::memcpy(instance, cursor, instance_size);
		info->instances.insert({instance->type_id, instance});
		cursor += instance_size;
	}
}

struct inline_info *inline_info__parse(std::span<std::byte> data) {
  struct inline_encoder__header *hdr = reinterpret_cast<struct inline_encoder__header *>(data.data());
  if (hdr->magic != 0xeb9f)
    return nullptr;
  if (hdr->inline_info_offset + hdr->inline_info_size > data.size())
    return nullptr;
  if (hdr->location_offset + hdr->location_size > data.size())
    return nullptr;

  auto *info = new struct inline_info;
  info->hdr = hdr;
  info->locations = reinterpret_cast<struct loc*>(data.data() + hdr->location_offset);
	inline_info__parse_instances(info);
  return info;
}
