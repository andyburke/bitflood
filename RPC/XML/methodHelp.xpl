<?xml version="1.0"?>
<!DOCTYPE methoddef SYSTEM "rpc-method.dtd">
<!--
    Generated automatically by make_method v1.09, Wed Aug  4 21:45:10 2004

    Any changes made here will be lost.
-->
<methoddef>
<name>system.methodHelp</name>
<version>1.2</version>
<signature>string string</signature>
<signature>array array</signature>
<help>
Return the help text (such as this) associated with the specified method(s).
If a STRING parameter specifying the method name is passed, the return value
will be a STRING. If multiple methods are queried by passing an ARRAY of
STRING values, then the return value will be an ARRAY of STRING values, as
well.
</help>
<code language="perl">
<![CDATA[
#!/usr/bin/perl
###############################################################################
#
#   Sub Name:       methodHelp
#
#   Description:    Retrieve any help text for the specified methods.
#
#   Arguments:      NAME      IN/OUT  TYPE      DESCRIPTION
#                   $srv      in      ref       Server object instance
#                   $arg      in      ref/sc    Listref or scalar specification
#
#   Globals:        None.
#
#   Environment:    None.
#
#   Returns:        Success:    string or listref
#                   Failure:    fault object
#
###############################################################################
sub methodHelp
{
    use strict;

    my $srv = shift;
    my $arg = shift;

    my $name = $srv->{method_name};
    my @list = (ref $arg) ? @$arg : ($arg);
    my @results = ();
    my $method;

    for (@list)
    {
        if (ref($method = $srv->get_method($_)) and (! $method->hidden))
        {
            push(@results, $method->help() || '');
        }
        else
        {
            return RPC::XML::fault->new(302, "$name: Method $_ unknown");
        }
    }

    return (ref $arg) ? \@results : $results[0];
}

__END__
]]></code>
</methoddef>
