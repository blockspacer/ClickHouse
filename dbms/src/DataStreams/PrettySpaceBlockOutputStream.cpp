#include <sys/ioctl.h>
#include <unistd.h>

#include <DB/Functions/FunctionsMiscellaneous.h>

#include <DB/DataStreams/PrettySpaceBlockOutputStream.h>


namespace DB
{


void PrettySpaceBlockOutputStream::write(const Block & block_)
{
	if (total_rows >= max_rows)
	{
		total_rows += block_.rows();
		return;
	}
	
	/// Будем вставлять суда столбцы с вычисленными значениями видимых длин.
	Block block = block_;
	
	size_t rows = block.rows();
	size_t columns = block.columns();

	Widths_t max_widths;
	Widths_t name_widths;
	calculateWidths(block, max_widths, name_widths);

	/// Не будем выравнивать по слишком длинным значениям.
	if (terminal_width > 80)
		for (size_t i = 0; i < columns; ++i)
			if (max_widths[i] > terminal_width / 2)
				max_widths[i] = terminal_width / 2;

	/// Имена
	for (size_t i = 0; i < columns; ++i)
	{
		if (i != 0)
			writeString("   ", ostr);

		const ColumnWithNameAndType & col = block.getByPosition(i);

		if (col.type->isNumeric())
		{
			for (ssize_t k = 0; k < std::max(0L, static_cast<ssize_t>(max_widths[i] - name_widths[i])); ++k)
				writeChar(' ', ostr);

			if (!no_escapes)
				writeString("\033[1;37m", ostr);
			writeEscapedString(col.name, ostr);
			if (!no_escapes)
				writeString("\033[0m", ostr);
		}
		else
		{
			if (!no_escapes)
				writeString("\033[1;37m", ostr);
			writeEscapedString(col.name, ostr);
			if (!no_escapes)
				writeString("\033[0m", ostr);

			for (ssize_t k = 0; k < std::max(0L, static_cast<ssize_t>(max_widths[i] - name_widths[i])); ++k)
				writeChar(' ', ostr);
		}
	}
	writeString("\n\n", ostr);

	for (size_t i = 0; i < rows && total_rows + i < max_rows; ++i)
	{
		for (size_t j = 0; j < columns; ++j)
		{
			if (j != 0)
				writeString("   ", ostr);

			const ColumnWithNameAndType & col = block.getByPosition(j);

			if (col.type->isNumeric())
			{
				size_t width = boost::get<UInt64>((*block.getByPosition(columns + j).column)[i]);
				for (ssize_t k = 0; k < std::max(0L, static_cast<ssize_t>(max_widths[j] - width)); ++k)
					writeChar(' ', ostr);
					
				col.type->serializeTextEscaped((*col.column)[i], ostr);
			}
			else
			{
				col.type->serializeTextEscaped((*col.column)[i], ostr);

				size_t width = boost::get<UInt64>((*block.getByPosition(columns + j).column)[i]);
				for (ssize_t k = 0; k < std::max(0L, static_cast<ssize_t>(max_widths[j] - width)); ++k)
					writeChar(' ', ostr);
			}
		}

		writeChar('\n', ostr);
	}

	total_rows += rows;
}


void PrettySpaceBlockOutputStream::writeSuffix()
{
	if (total_rows >= max_rows)
	{
		writeString("\nShowed first ", ostr);
		writeIntText(max_rows, ostr);
		writeString(".", ostr);
	}
}

}
