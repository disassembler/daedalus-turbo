/* This file is part of Daedalus Turbo project: https://github.com/sierkov/daedalus-turbo/
 * Copyright (c) 2022-2023 Alex Sierkov (alex dot sierkov at gmail dot com)
 * This code is distributed under the license specified in:
 * https://github.com/sierkov/daedalus-turbo/blob/main/LICENSE */
#include <dt/cli/http-api.hpp>
#include <dt/cli/pay-history.hpp>
#include <dt/cli/stake-history.hpp>
#include <dt/cli/sync-http.hpp>
#include <dt/cli/sync-local.hpp>
#include <dt/cli/truncate.hpp>
#include <dt/cli/tx-info.hpp>

int main(int argc, char **argv)
{
    using namespace daedalus_turbo::cli;
    return run(argc, argv, make_command_list(
        std::make_unique<http_api::cmd>(),
        std::make_unique<pay_history::cmd>(),
        std::make_unique<stake_history::cmd>(),
        std::make_unique<sync_http::cmd>(),
        std::make_unique<sync_local::cmd>(),
        std::make_unique<truncate::cmd>(),
        std::make_unique<tx_info::cmd>()
    ));
}