<?xml version="1.0"?>
<!DOCTYPE methoddef SYSTEM "rpc-method.dtd">
<!--
    Generated automatically by make_method v1.09, Wed Aug  4 21:45:10 2004

    Any changes made here will be lost.
-->
<methoddef>
<name>system.status</name>
<version>1.1</version>
<signature>struct</signature>
<help>
Report on the various status markers of the server itself. The return value is
a STRUCT with the following members:

        Key         Type     Value

        host        STRING   Name of the (possibly virtual) host name to which
                             requests are sent.
        port        INT      TCP/IP port the server is listening on.
        name        STRING   The name of the server software, as it identifies
                             itself in transport headers.
        version     STRING   The software version. Note that this is defined as
                             a STRING, not a DOUBLE, to allow for non-numeric
                             values.
        path        STRING   URL path portion, for use when sending POST
                             request messages.
        date        ISO8601  The current date and time on the server, as an
                             ISO 8601 date string.
        date_int    INT      The current date as a UNIX time() value. This is
                             encoded as an INT rather than the dateTime.int
                             type, so that it is readable by older clients.
        started     ISO8601  The date and time when the current server started
                             accepting connections, as an ISO 8601 string.
        started_int
                    INT      The server start-time as a UNIX time() value. This
                             is also encoded as INT for the same reasons as
                             the "date_int" value above.
        total_requests
                    INT      Total number of requests served thus far
                             (including the current one). This will not include
                             requests for which there was no matching method,
                             or HTTP-HEAD requests.
        methods_known
                    INT      The number of different methods the server has
                             registered for serving requests.
</help>
<code language="perl">
<![CDATA[
#!/usr/bin/perl
###############################################################################
#
#   Sub Name:       status
#
#   Description:    Create a status-reporting struct and returns it.
#
#   Arguments:      NAME      IN/OUT  TYPE      DESCRIPTION
#                   $srv      in      ref       Server object instance
#
#   Globals:        None.
#
#   Environment:    None.
#
#   Returns:        hashref
#
###############################################################################
sub status
{
    use strict;

    my $srv = shift;

    my $status = {};
    my $time = time;
    my $URI;

    require URI;

    $status->{name} = ref($srv);
    $status->{version} = new RPC::XML::string $srv->version;
    $status->{host} = $srv->host || $srv->{host} || '';
    $status->{port} = $srv->port || $srv->{port} || '';
    $status->{path} = new RPC::XML::string $srv->path;
    $status->{date} = RPC::XML::datetime_iso8601
        ->new(RPC::XML::time2iso8601($time));
    $status->{started} = RPC::XML::datetime_iso8601
        ->new(RPC::XML::time2iso8601($srv->started));
    $status->{date_int} = $time;
    $status->{started_int} = $srv->started;
    $status->{total_requests} = $srv->requests() + 1;
    $status->{methods_known} = scalar($srv->list_methods);

    $status;
}

__END__
]]></code>
</methoddef>
