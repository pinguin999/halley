#include "test_stage.h"

using namespace Halley;

TestStage::TestStage()
{
}

void TestStage::init()
{

}

void TestStage::onFixedUpdate(Time time)
{
	auto key = getInputAPI().getKeyboard();
	if (key->isButtonDown(Keys::Esc)) {
		getCoreAPI().quit();
	}

	updateNetwork();
}

void TestStage::onRender(RenderContext& context) const
{
	context.bind([&] (Painter& painter)
	{
		painter.clear(Colour(0));
	});
}

void TestStage::updateNetwork()
{
	auto key = getInputAPI().getKeyboard();

	if (!network) {
		if (key->isButtonPressed(Keys::S)) {
			// Server
			network = std::make_unique<NetworkService>(4113);
			network->setAcceptingConnections(true);
			std::cout << "Listening..." << std::endl;
		}
		else if (key->isButtonPressed(Keys::C)) {
			// Client
			network = std::make_unique<NetworkService>(0);
			setConnection(network->connect("127.0.0.1", 4113));			
			std::cout << "Connecting as client." << std::endl;
		}
	} else {
		network->update();

		auto conn = network->tryAcceptConnection();
		if (conn) {
			setConnection(conn);
			std::cout << "Client connected." << std::endl;
		}

		if (connection) {
			if (key->isButtonPressed(Keys::Space)) {
				connection->send(NetworkPacket("hello world!", 13));
			}

			NetworkPacket received;
			while (connection->receive(received)) {
				char buffer[64];
				received.copyTo(buffer, 64);
				std::cout << "Received: " << buffer << std::endl;
			}
		}

		network->update();
	}
}

void TestStage::setConnection(std::shared_ptr<Halley::IConnection> conn)
{
	connection = std::make_shared<InstabilitySimulator>(conn, 0.5f, 0.1f, 0.1f);
}
