#include <string.h>
#include <stdexcept>

#include "app.h"
#include "graph.h"
#include "monitor.h"
#include "request.h"
#include "simulation.h"


app::app(int argc, const char** argv) {
	level = 0;
	username = "hakker1337";
	password = "test1234";

	for (int i = 0; i < argc; i++) {
		if( strcmp(argv[i], "--level") == 0) {
			if (i + 1 >= argc)
				throw std::runtime_error("missing level");

			level = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "--user") == 0) {
			if (i + 1 >= argc)
				throw std::runtime_error("missing username");

			username = argv[++i];
		}
		else if (strcmp(argv[i], "--password") == 0) {
			if (i + 1 >= argc)
				throw std::runtime_error("missing password");

			password = argv[++i];
		}
	}
}

void app::parse(std::string& data, int* level, int* width, int* height, std::string* board) const {
	std::smatch m;
	std::regex_search(data, m, r_level);
	*level = atoi(m.str(1).c_str());

	if (!std::regex_search(data, m, r_vars))
		throw std::runtime_error("Invalid Data.");

	std::string match = m.str(1);

	std::regex_search(match, m, r_width);
	*width = atoi(m.str(1).c_str());

	std::regex_search(match, m, r_height);
	*height = atoi(m.str(1).c_str());

	std::regex_search(match, m, r_board);
	*board = m.str(1);
}

bool app::solve(const board& board, std::string* path) {
	simulation s(board);
	try {
		s.dfs(path);
		return true;
	}
	catch (std::exception e) {
		monitor::_emit(monitor::event("exception") << "process" << e.what());
		return false;
	}

}

void app::parseline(std::fstream& file, int& width, int& height, std::string& data) const {
	char buffer[1024];
	file.getline(buffer, sizeof(buffer));
	std::cmatch m;
	if (!std::regex_search(buffer, m, std::regex("^(\\d+)\\s+(\\d+)\\s+([a-z\\.]+)$")) )
		throw std::runtime_error("could not parse line");

	width = atoi(m.str(1).c_str());
	height = atoi(m.str(2).c_str());
	data = m.str(3);
}

void app::run() {

	int width, height;
	std::string data, path;
	char url[1024];

	//std::fstream levels("levels.txt", std::fstream::in);
	sprintf_s(url, sizeof(url), "/brick/index.php?name=%s&password=%s&gotolevel=%d", username, password, level);
	data = request("www.hacker.org", url);
	while ( true ) {
		//parseline(levels, width, height, data);

		parse(data, &level, &width, &height, &data);
		
		char buffer[512];
		sprintf_s(buffer, sizeof(buffer), "output/level%02d.graph", level);
		graph* g = graph::create(buffer);

		timer t;
		solve(board(width, height, data.c_str()), &path);
		
		monitor::_emit(monitor::event("solved") << level++ << int(t.get() * 1000) << path);

		g->release();

		sprintf_s(url, sizeof(url), "/brick/index.php?name=%s&password=%s&path=%s", username, password, path.c_str());
		data = request("www.hacker.org", url);
		level += 1;
	}
}