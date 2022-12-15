#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include "ecsact/parse.h"
#include "ecsact/interpret/eval.h"

#include "./messages.hh"
#include "./interfaces.hh"

namespace ecsact_lsp {

struct document_state {
	std::optional<ecsact_package_id>           package_id;
	std::vector<ecsact_package_id>             imports;
	std::vector<std::vector<ecsact_statement>> parse_stacks;
	std::string                                text;
};

struct workspace_state {
	std::string                                     uri;
	std::unordered_map<std::string, document_state> documents;
};

template<send_interface Send>
class workspace_manager {
	std::unordered_map<std::string, workspace_state> _workspaces;
	Send&&                                           send;

public:
	workspace_manager(Send&& send) : send(std::forward<Send>(send)) {
	}

	auto add_workspace(workspace_folder ws) -> void {
		send.show_message(message_type::error, "Add Workspace: " + ws.uri);
	}

	auto remove_workspace(workspace_folder ws) -> void {
		send.show_message(message_type::error, "Remove Workspace: " + ws.uri);
	}

	auto add_document(std::string uri, int initial_version, std::string text)
		-> void {
		send.show_message(message_type::error, "Add Document: " + uri);
	}

	auto update_document(std::string uri, int version, std::string text) -> void {
		send.show_message(message_type::error, "Update Document: " + uri);
	}

	auto remove_document(std::string uri) -> void {
		send.show_message(message_type::error, "Remove Document: " + uri);
	}
};

template<send_interface Send>
workspace_manager(Send&&) -> workspace_manager<Send>;

} // namespace ecsact_lsp
