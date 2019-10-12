#include "connection.hpp"


namespace nats_asio {



/*
void connection::load_certificates(const ssl_config& conf)
{
	m_ssl_ctx.set_options(
		boost::asio::ssl::context::default_workarounds |
		boost::asio::ssl::context::no_sslv2 |
		boost::asio::ssl::context::no_sslv3 |
		boost::asio::ssl::context::no_tlsv1 |
		boost::asio::ssl::context::single_dh_use |
		boost::asio::ssl::context::tls_client
	);

	if (conf.ssl_verify)
	{
		m_ssl_ctx.set_verify_mode(boost::asio::ssl::verify_peer);
	}
	else
	{
		m_ssl_ctx.set_verify_mode(boost::asio::ssl::verify_none);
	}

	if (!conf.ssl_cert.empty())
	{
		m_ssl_ctx.use_certificate(boost::asio::buffer(conf.ssl_cert.data(), conf.ssl_cert.size()), boost::asio::ssl::context::file_format::pem);
	}

	if (!conf.ssl_ca.empty())
	{
		m_ssl_ctx.use_certificate_chain(boost::asio::buffer(conf.ssl_ca.data(), conf.ssl_ca.size()));
	}

	if (!conf.ssl_dh.empty())
	{
		m_ssl_ctx.use_tmp_dh_file(conf.ssl_dh);
	}

	if (!conf.ssl_key.empty())
	{
		m_ssl_ctx.use_private_key(boost::asio::buffer(conf.ssl_key.data(), conf.ssl_key.size()), boost::asio::ssl::context::file_format::pem);
	}
}*/




































}


