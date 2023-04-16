vim.lsp.set_log_level("TRACE")

local cmd = vim.fn.getcwd() .. "/bazel-bin/ecsact_lsp_server"

if vim.fn.has('win32') then
	cmd = cmd .. ".exe"
end

local diagnostics_examples = vim.split(
	vim.fn.glob(vim.fn.getcwd() .. '/test/diagnostics_examples/*.ecsact'),
	'\n'
)

local view_time = 5000

for i, file in ipairs(diagnostics_examples) do
	vim.defer_fn(function()
		vim.cmd('edit ' .. file)
		vim.bo.filetype = "ecsact"

		vim.lsp.start({
			name = 'ecsact',
			cmd = {cmd, "--stdio"},
			root_dir = vim.fn.getcwd(),
		})
	end, view_time * (i - 1))
end

vim.defer_fn(function()
	vim.cmd('qa!')
end, view_time * (#diagnostics_examples))

