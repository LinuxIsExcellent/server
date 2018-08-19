#! /usr/bin/php

<?php
if ($argc !== 3) {
    dump_help();
}

function dump_help() {
    $help = <<<CODE
帮助:
    用于自动生成LUA代码

    ./metadata_gen.php {protocol.xml与metadata.xml文件所在路径} {版本号}

    例如: ./metadata_gen.php /opt/work/sg/protocols v3.1.0
CODE;
    echo $help;
    echo PHP_EOL;
    exit;
}

$xmlDir = $argv[1];
$version = $argv[2];

$xml_file = "$xmlDir/protocol_$version.xml";
if (!file_exists($xml_file)) {
    echo "文件不存在:" . $xml_file;
    exit;
}

# 生成元数据对应的代码
$file_metadata = '';


$xml = simplexml_load_file($xml_file);

function convert_name($vn)
{
	$ret = "";
	$arr = str_split($vn);
	for ($i = 0; $i < count($arr); ++$i) {
		if ($i > 1 && $i < count($arr) - 2) {
			if (ctype_lower($arr[$i-1]) && ctype_upper($arr[$i]) && ctype_lower($arr[$i+1])) {
				$ret .= '_';
			}
		}
		$ret .= $arr[$i];
	}

	return strtoupper($ret);
}

// 生成枚举
function gen_enum(&$file_metadata, $enum)
{
    $prefix = str_replace('_TYPE', '', convert_name($enum['name']));
    $prefix = str_replace('TYPE', '', $prefix);
    
    $file_metadata .= "-----\n";
    $file_metadata .= "-- {$enum['desc']}\n";
    $file_metadata .= "M.{$enum['name']} = {\n";
    foreach ($enum->item as $item) {
        $file_metadata .= "    {$item['code']} = {$item['value']},        -- {$item}\n";
    }
    $file_metadata .= "}\n\n";
}

foreach ($xml->enum as $enum) {
    gen_enum($file_metadata, $enum);
}

$file_metadata .= "\n\n";


// 生成协议号
$protocol_sc = "\n";
$protocol_cs = "\n";
$protocol_cs_map = "\nM.CS_MAP = {}\n";

function gen_protocol(&$code, $p, $type)
{
    $code  .= "{$type}{$p['code']} = {$p['value']}\n";
}
function gen_protocol_map(&$code, $p, $type)
{
    $code  .= "{$type}[{$p['value']}] = {$p['value']}\n";
}

foreach ($xml->flow as $flow) {
    foreach($flow->protocol as $protocol) {
        if (strtoupper($protocol['type']) == 'SC') {
            gen_protocol($protocol_sc, $protocol, 'M.SC_');
        } else {
            gen_protocol($protocol_cs, $protocol, 'M.CS_');
            gen_protocol_map($protocol_cs_map, $protocol, 'M.CS_MAP');
        }
    }
}

$protocol_sc .= "\n";
$protocol_cs .= "\n";

$file_protocol = <<<CODE

-- 由工具自动生成，请不要手动修改

local M = {}

{$file_metadata}

    {$protocol_cs}
    {$protocol_sc}

    {$protocol_cs_map}

return M
\n
CODE;

file_put_contents(dirname(__FILE__) . '/script/protocol.lua', $file_protocol);
echo 'LUA 代码生成成功', PHP_EOL;

?>
