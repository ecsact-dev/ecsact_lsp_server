#pragma once

#include <unordered_map>
#include <unordered_set>
#include <set>
#include <string>
#include <vector>
#include <string_view>
#include <ranges>
#include "ecsact/parse.h"
#include "ecsact/interpret/eval.h"
#include "ecsact/runtime/dynamic.h"
#include "magic_enum.hpp"

#include "./messages.hh"
#include "./interfaces.hh"

namespace ecsact_lsp {

struct document_state {
	std::optional<ecsact_package_id>           package_id;
	std::vector<ecsact_package_id>             imports;
	std::vector<std::vector<ecsact_statement>> parse_stacks;
	std::vector<ecsact_parse_status>           parse_statuses;
	std::string                                full_text;
	std::string_view                           next_parse_view;
};

struct workspace_state {
	std::string                     uri;
	std::unordered_set<std::string> document_uris;
};

inline auto get_source_range(
	const std::string&         full_text,
	const ecsact_statement_sv& loc
) -> range {
	auto r = range{};
	auto index = 0;
	auto cptr = full_text.data();

	while(index < full_text.size() && cptr != loc.data) {
		if(*cptr == '\n') {
			r.start.line += 1;
			r.start.character = 0;
		} else {
			r.start.character += 1;
		}

		++cptr;
		++index;
	}

	r.end = r.start;
	r.end.character += loc.length;

	return r;
}

inline auto parse_stack(
	std::string_view               text,
	ecsact_parse_status&           status,
	std::vector<ecsact_statement>& stack
) -> std::string_view {
	auto& statement = stack.emplace_back();
	auto  context = stack.size() == 1 ? nullptr : &stack[stack.size() - 2];

	auto read_amount = ecsact_parse_statement(
		text.data(),
		static_cast<int>(text.size()),
		context,
		&statement,
		&status
	);

	if(ecsact_is_error_parse_status_code(status.code)) {
		return {};
	}

	return text.substr(read_amount);
}

template<send_interface Send>
class workspace_manager {
	std::unordered_map<std::string, workspace_state> _workspaces;
	std::unordered_map<std::string, document_state>  _documents;
	Send&&                                           send;

	auto _parse_document(std::string uri, int version, document_state& doc) {
		doc.parse_stacks.clear();
		doc.parse_statuses.clear();

		auto diagnostics = std::vector<diagnostic>{};

		while(!doc.next_parse_view.empty()) {
			auto& status = doc.parse_statuses.emplace_back();
			auto& stack = doc.parse_stacks.emplace_back();

			if(doc.parse_stacks.size() > 1) {
				auto last_status = doc.parse_statuses[doc.parse_statuses.size() - 2];
				stack = doc.parse_stacks[doc.parse_stacks.size() - 2];
				if(last_status.code == ECSACT_PARSE_STATUS_OK) {
					stack.pop_back();
				}
			}

			doc.next_parse_view = parse_stack(doc.next_parse_view, status, stack);
		}

		for(auto& status : doc.parse_statuses) {
			if(ecsact_is_error_parse_status_code(status.code)) {
				auto r = get_source_range(doc.full_text, status.error_location);

				diagnostics.push_back(diagnostic{
					.range = r,
					.severity = diagnostic_severity::error,
					.message{magic_enum::enum_name(status.code)},
				});
			}
		}

		send.send_notification(
			"textDocument/publishDiagnostics",
			nlohmann::json{
				{"uri", uri},
				{"version", version},
				{"diagnostics", diagnostics},
			}
		);
	}

	auto _interpret_document(std::string uri, int version, document_state& doc) {
		using std::views::drop;
		using namespace std::string_literals;

		if(doc.parse_stacks.empty()) {
			return;
		}

		if(doc.package_id) {
			ecsact_destroy_package(*doc.package_id);
			doc.package_id = std::nullopt;
		}

		auto  diagnostics = std::vector<diagnostic>{};
		auto& package_parse_stack = doc.parse_stacks.front();

		if(package_parse_stack.empty()) {
			return;
		}

		auto& package_statement = package_parse_stack.front();

		if(package_statement.type != ECSACT_STATEMENT_PACKAGE) {
			diagnostics.push_back(diagnostic{
				.range{},
				.severity = diagnostic_severity::warning,
				.message = "First statement must be a package statement",
			});
		} else {
			doc.package_id = ecsact_eval_package_statement( //
				&package_statement.data.package_statement
			);
		}

		if(doc.package_id) {
			auto eval_statement_tracker = std::set<int32_t>{};

			// Evaluate all statements in stacks except first (package)
			for(auto& parse_stack : doc.parse_stacks | drop(1)) {
				for(auto idx = 0; parse_stack.size() > idx; ++idx) {
					auto& statement = parse_stack[idx];
					if(eval_statement_tracker.contains(statement.id)) {
						continue;
					}
					eval_statement_tracker.insert(statement.id);

					auto eval_err = ecsact_eval_statement(
						*doc.package_id,
						static_cast<int32_t>(parse_stack.size()),
						parse_stack.data() + idx
					);

					if(eval_err.code != ECSACT_EVAL_OK) {
						auto r = get_source_range(doc.full_text, eval_err.relevant_content);
						auto message = std::string{magic_enum::enum_name(eval_err.code)};
						if(eval_err.relevant_content.length > 0) {
							message += " ";
							message += std::string_view{
								eval_err.relevant_content.data,
								static_cast<size_t>(eval_err.relevant_content.length),
							};
						}

						diagnostics.push_back(diagnostic{
							.range = r,
							.severity = diagnostic_severity::error,
							.message = message,
						});
					}
				}
			}
		}

		if(!diagnostics.empty()) {
			send.send_notification(
				"textDocument/publishDiagnostics",
				nlohmann::json{
					{"uri", uri},
					{"version", version},
					{"diagnostics", diagnostics},
				}
			);
		}
	}

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
		auto& doc = (_documents[uri] = {});
		doc.full_text = std::move(text);
		doc.next_parse_view = doc.full_text;
		_parse_document(uri, initial_version, doc);
		_interpret_document(uri, initial_version, doc);
	}

	auto update_document(std::string uri, int version, std::string text) -> void {
		auto& doc = (_documents[uri] = {});
		doc.full_text = std::move(text);
		doc.next_parse_view = doc.full_text;
		_parse_document(uri, version, doc);
		_interpret_document(uri, version, doc);
	}

	auto remove_document(std::string uri) -> void {
	}
};

template<send_interface Send>
workspace_manager(Send&&) -> workspace_manager<Send>;

} // namespace ecsact_lsp
