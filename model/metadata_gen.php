#! /usr/bin/php

<?php

if ($argc !== 3) {
    dump_help();
}

function dump_help() {
    $help = <<<CODE
帮助:
    用于自动生成C++代码

    ./metadata_gen.php {protocol_*.xml文件所在路径} {版本号}

    例如: ./metadata_gen.php /opt/s3/protocols v3.1.0
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
$file_metadata = <<<CODE
#ifndef MODEL_METADATA_H
#define MODEL_METADATA_H

namespace model
{

CODE;

$xml = simplexml_load_file($xml_file);


// 生成枚举
function gen_enum(&$file_metadata, $enum)
{
    $file_metadata .= "    // {$enum['desc']}\n";
    $file_metadata .= "    enum class {$enum['name']} \n    {\n";
    foreach ($enum->item as $item) {
        $file_metadata .= "        {$item['code']} = {$item['value']},        // {$item}\n";
    }
    $file_metadata .= "    };\n\n";

    $file_metadata .= "    bool inline {$enum['name']}_IsValid(int v)\n";
    $file_metadata .= "    {\n";
    $file_metadata .= "        switch (v) {\n";
    foreach ($enum->item as $item) {
        if ($item['code'] != 'SIZE') // 剔除内部保留的SIZE
        {
            $file_metadata .= "            case {$item['value']}:\n";
        }
    }
    $file_metadata .= "                return true;\n";
    $file_metadata .= "            default:\n";
    $file_metadata .= "                return false;\n";
    $file_metadata .= "        }\n";
    $file_metadata .= "    }\n\n";
}

foreach ($xml->enum as $enum) {
    gen_enum($file_metadata, $enum);
}

$file_metadata .= "}\n\n#endif";

file_put_contents(dirname(__FILE__) . '/model/metadata.h', $file_metadata);

// 生成协议号
$protocol_sc = "enum class SC {\n";
$protocol_cs = "enum class CS {\n";

function gen_protocol(&$code, $p)
{
    $code  .= "        {$p['code']} = {$p['value']},\n";
}

foreach ($xml->flow as $flow) {
    foreach($flow->protocol as $protocol) {
        if (strtoupper($protocol['type']) == 'SC') {
            gen_protocol($protocol_sc, $protocol);
        } else {
            gen_protocol($protocol_cs, $protocol);
        }
    }
}

$protocol_sc .= "    };\n";
$protocol_cs .= "    };\n";

$file_protocol = <<<CODE
#ifndef MODEL_PROTOCOL_H
#define MODEL_PROTOCOL_H

namespace model
{
    {$protocol_cs}
    {$protocol_sc}
}

#endif
\n
CODE;

file_put_contents(dirname(__FILE__) . '/model/protocol.h', $file_protocol);
echo '代码生成成功！', PHP_EOL;

?>
