<?xml version="1.0"?>
<!DOCTYPE methoddef SYSTEM "rpc-method.dtd">
<!--
    Generated automatically by make_method v1.09, Wed Aug  4 21:45:09 2004

    Any changes made here will be lost.
-->
<methoddef>
<name>system.identity</name>
<version>1.1</version>
<signature>string</signature>
<help>
Return the server name and version as a string
</help>
<code language="perl">
<![CDATA[
#!/usr/bin/perl
###############################################################################
#
#   Sub Name:       identity
#
#   Description:    Simply returns the server's identity as a string
#
#   Arguments:      First arg is server instance
#
#   Globals:        None.
#
#   Returns:        string
#
###############################################################################
sub identity
{
    use strict;

    $_[0]->product_tokens;
}

__END__
]]></code>
</methoddef>
