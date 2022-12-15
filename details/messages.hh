#pragma once

#include <string>
#include "nlohmann/json.hpp"

namespace ecsact_lsp {

enum class request_id : int;

enum class message_type {
	error = 1,
	warning = 2,
	info = 3,
	log = 4,
};

enum class lsp_error_code {
	parse_error = -32700,
	invalid_request = -32600,
	method_not_found = -32601,
	invalid_param = -32602,
	internal_error = -32603,
};

struct workspace_folder {
	std::string uri;
	std::string name;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(workspace_folder, uri, name);
};

struct message_action_item {
	std::string title;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(message_action_item, title);
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#textDocumentIdentifier
 */
struct text_document_identifier {
	std::string uri;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(text_document_identifier, uri);
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#versionedTextDocumentIdentifier
 */
struct versioned_text_document_identifier {
	std::string uri;
	int         version;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(
		versioned_text_document_identifier,
		uri,
		version
	);
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#textDocumentItem
 */
struct text_document_item {
	std::string uri;
	std::string languageId;
	int         version;
	std::string text;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(
		text_document_item,
		uri,
		languageId,
		version,
		text
	);
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#textDocumentContentChangeEvent
 */
struct full_text_document_content_change_event {
	std::string text;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(full_text_document_content_change_event, text);
};

} // namespace ecsact_lsp
