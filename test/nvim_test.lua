-- File for testing in neovim
-- :luafile %

vim.lsp.set_log_level("TRACE")

local cmd = vim.fn.getcwd() .. "/bazel-bin/ecsact_lsp_server"

if vim.fn.has('win32') then
	cmd = cmd .. ".exe"
end

require("lspconfig").ecsact.setup {
	root_dir = function()
		return vim.fn.getcwd()
	end,
	cmd = {cmd},
}

vim.cmd [[:LspStop ecsact]]

